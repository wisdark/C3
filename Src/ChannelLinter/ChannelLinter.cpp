#include "stdafx.h"
#include "ChannelLinter.h"

#include "Core/Profiler.h"

namespace FSecure::C3::Linter
{
	namespace
	{
		/// Return Interface metadata
		/// @param channel name - case sensitive
		/// @returns Interface metadata
		/// @throws std::runtime_error if channel with given name was not registered
		auto const& GetChannelInfo(std::string_view channelName)
		{
			try
			{
				return InterfaceFactory::Instance().Find<AbstractChannel>(channelName)->second;
			}
			catch (std::runtime_error & e)
			{
				// registered interface with this name was not registered, try finding one using case insensitive comparison
				auto const& channels = InterfaceFactory::Instance().GetMap<AbstractChannel>();
				std::string channelNameStr{ channelName };
				auto it = std::find_if(cbegin(channels), cend(channels), [&channelNameStr](auto&& ch)
					{
						return _stricmp(channelNameStr.c_str(), ch.second.m_Name.c_str()) == 0;
					});

				if (it != cend(channels))
					throw std::runtime_error(e.what() + ". Did you mean "s + it->second.m_Name + '?');
				else
					throw;
			}
		}

		/// Get Channel capability supplemented with build-in capability
		/// @param channel metadata
		/// @returns Channel capability supplemented with build-in capability
		auto GetChannelCapability(InterfaceFactory::InterfaceData<AbstractChannel> const& channelInfo)
		{
			try
			{
				auto ret = json::parse(channelInfo.m_Capability);
				if(!ret.contains("create"))
				{
					std::cout << "[Warning] create property does not exist, generating default one." << std::endl;
					C3::Core::Profiler::Gateway::EnsureCreateExists(ret);
				}
				C3::Core::Profiler::Gateway::AddBuildInCommands(ret, true);
				return ret;
			}
			catch (json::parse_error& e)
			{
				throw std::runtime_error("Failed to parse channel's capability json. "s + e.what());
			}
		}

		/// Get command id from string (allows negative inputs e.g "-2" for Close)
		/// @param command Id string
		/// @returns command Id
		/// @throws std::invalid_argument
		uint16_t GetCommandId(std::string const& commandId)
		{
			// allow negative inputs
			auto commandIdL = std::stoi(commandId);
			return static_cast<uint16_t>(commandIdL);
		}
	}

	ChannelLinter::ChannelLinter(AppConfig config) :
		m_Config(std::move(config)),
		m_ChannelData(GetChannelInfo(*m_Config.m_ChannelName)),
		m_ChannelCapability(GetChannelCapability(m_ChannelData)),
		m_CreateForm{ [this]
			{
				std::cout << "Parsing create arguments ... " << std::flush;
				decltype(m_CreateForm) form(m_ChannelCapability.at("/create/arguments"_json_pointer));
				std::cout << "OK" << std::endl;
				return form;
			}()
		}
	{
		std::cout << "Parsing command definitions ... " << std::flush;
		for (auto&& command : m_ChannelCapability.at("commands"))
		{
			auto parsedCommand = ChannelCommand(command);
			auto id = parsedCommand.m_Id;
			if(m_ChannelCommands.count(id))
				throw std::invalid_argument("Command with id = " + std::to_string(id) + " already exists.");
			 m_ChannelCommands.emplace(id, std::move(parsedCommand));
		}
		std::cout << "OK" << std::endl;
		std::cout << "Registered commands: \nid\tname\n";
		for (auto [k, v] : m_ChannelCommands)
			std::cout << k << '\t' << v.m_Name << '\n';
		std::cout << std::flush;
	}

	void ChannelLinter::Process()
	{
		std::shared_ptr<MockDeviceBridge> channel;
		if (m_Config.ShouldCreateChannel())
		{
			std::cout << "Creating channel ... " << std::flush;
			channel = MakeChannel(*m_Config.m_ChannelArguments);
			std::cout << "OK" << std::endl;
		}

		if (m_Config.m_TestChannelIO)
		{
			assert(channel); // First channel should already be created
			std::cout << "Creating complementary channel ... " << std::flush;
			auto complementaryArgs = GetComplementaryChannelArgs();
			auto complementaryChannel = MakeChannel(complementaryArgs);
			assert(complementaryChannel);
			std::cout << "OK" << std::endl;

			TestChannelIO(*channel.get(), *complementaryChannel.get(), m_Config.m_OverlappedIO);
		}

		if (m_Config.m_Command)
		{
			assert(channel); // First channel should already be created
			TestCommand(channel);
		}
	}

	std::shared_ptr<FSecure::C3::Linter::MockDeviceBridge> ChannelLinter::MakeChannel(StringVector const& channnelArguments) const
	{
		Form form(m_ChannelCapability.at("/create/arguments"_json_pointer));
		auto createParams = form.Fill(channnelArguments);
		auto blob = FSecure::C3::Core::Profiler::TranslateArguments(createParams);
		return MakeChannel(blob);
	}

	std::shared_ptr<MockDeviceBridge> ChannelLinter::MakeChannel(ByteView blob) const
	{
		auto channelBridge = std::make_shared<MockDeviceBridge>(m_ChannelData.m_Builder(blob));
		channelBridge->OnAttach();
		return channelBridge;
	}

	void ChannelLinter::TestChannelIO(MockDeviceBridge& channel, MockDeviceBridge& complementary, bool overlapped)
	{
		TestEcho(channel, complementary);
		TestChannelMTU(channel, complementary, overlapped);

		if (!overlapped)
			TestChannelOrder(channel, complementary);
	}

	void ChannelLinter::TestEcho(MockDeviceBridge& channel, MockDeviceBridge& complementary)
	{
		size_t packetLen = 8;
		std::cout << "Testing channel echo with " << packetLen << " bytes of data ... " << std::flush;
		auto data = ByteVector(FSecure::Utils::GenerateRandomData(packetLen));
		if (channel.GetDevice()->OnSendToChannelInternal(data) != data.size())
			throw std::runtime_error{ "Failed to send echo packet" };
		auto echo = std::static_pointer_cast<FSecure::C3::AbstractChannel>(channel.GetDevice())->OnReceiveFromChannelInternal();
		if (!echo.empty())
			throw std::runtime_error{ "Channel read it's own message" };

		// clean channel
		auto received = std::static_pointer_cast<FSecure::C3::AbstractChannel>(complementary.GetDevice())->OnReceiveFromChannelInternal();
		if (received.empty() || received.size() > 1 || received[0] != data)
			throw std::runtime_error{ "Receive error" };

		std::cout << "OK" << std::endl;
	}

	void ChannelLinter::TestChannelMTU(MockDeviceBridge& channel, MockDeviceBridge& complementary, bool overlapped)
	{
		for (size_t packetLen : { 8, 64, 1024, 1024 * 1024})
		{
			std::cout << "Testing channel with " << packetLen << " bytes of data ... " << std::flush;

			auto data = ByteVector(FSecure::Utils::GenerateRandomData(packetLen));
			auto passed = overlapped ? TestOverlapped(channel, complementary, data) : TestSequential(channel, complementary, data);
			if (!passed)
				throw std::exception("Data sent and received mismatch");

			std::cout << "OK" << std::endl;
		}
	}

	void ChannelLinter::TestChannelOrder(MockDeviceBridge& channel, MockDeviceBridge& complementary)
	{
		constexpr auto  numberOfTests = 10;
		constexpr auto packetSize = 64;
		std::vector<ByteVector> sent;

		std::cout << "Testing channel order with " << numberOfTests << " packets of " << packetSize << " bytes of data ... " << std::flush;

		for (auto i = 0; i < numberOfTests; ++i)
		{
			sent.push_back(FSecure::Utils::GenerateRandomData(packetSize));
			channel.Send(sent[i]);
		}

		auto received = complementary.Receive(sent.size());
		received.resize(sent.size());

		if (sent != received)
			throw std::exception("Data sent and received mismatch");

		std::cout << "OK" << std::endl;
	}


	bool ChannelLinter::TestOverlapped(MockDeviceBridge& channel, MockDeviceBridge& complementary, ByteView data)
	{
		auto sender = channel.GetChunkSender(data);
		auto receiver = complementary.GetChunkReceiver();

		for (auto noProgressCounter = 0; noProgressCounter < 10; ++noProgressCounter)
		{
			if (!sender.IsDone() && sender.Send())
				noProgressCounter = 0;

			if (receiver.Receive())
				noProgressCounter = 0;

			if (auto && packets = receiver.GetPackets(); !packets.empty())
				return ByteView{ packets[0] } == data;
		}

		return false;
	}

	bool ChannelLinter::TestSequential(MockDeviceBridge& channel, MockDeviceBridge& complementary, ByteView data)
	{
		channel.Send(data);
		auto received = complementary.Receive();
		return data == ByteView{ received[0] };
	}


	void ChannelLinter::TestCommand(std::shared_ptr<MockDeviceBridge> const& channel)
	{
		assert(m_Config.m_Command);
		std::cout << "Executing command ... " << std::flush;
		auto binaryCommand = TranslateCommand(*m_Config.m_Command);
		channel->RunCommand(binaryCommand);
		std::cout << "OK" << std::endl;
	}

	FSecure::ByteVector ChannelLinter::TranslateCommand(StringVector const& commandParams)
	{
		uint16_t commandId = GetCommandId(commandParams.at(0));

		if (!m_ChannelCommands.count(commandId))
			throw std::runtime_error("Failed to find a command with id: " + std::to_string(commandId));

		Form commandForm = m_ChannelCommands.at(commandId).m_ArgumentsForm;
		json command
		{
			{ "id", commandId},
			{ "arguments", commandForm.Fill({begin(commandParams) + 1, end(commandParams)})}, // + 1 to omit command id}
		};
		return C3::Core::Profiler::TranslateCommand(command);
	}

	StringVector ChannelLinter::GetComplementaryChannelArgs() const
	{
		Form form(m_ChannelCapability.at("/create/arguments"_json_pointer));
		return m_Config.m_ComplementaryChannelArguments ? *m_Config.m_ComplementaryChannelArguments : form.GetComplementaryArgs(*m_Config.m_ChannelArguments);
	}

	ChannelLinter::ChannelCommand::ChannelCommand(json const& commandDefinition) : m_ArgumentsForm(commandDefinition.at("arguments"))
	{
		if (!commandDefinition.contains("id"))
			throw std::invalid_argument{ "Command definition must contain 'id' property. Invalid command:\n" + commandDefinition.dump(4) };
		m_Id = commandDefinition["id"].get<uint16_t>();
		if (!commandDefinition.contains("name"))
			throw std::invalid_argument{ "Command definition id = " + std::to_string(m_Id) + " must contain 'name' property" };
		m_Name = commandDefinition["name"].get<std::string>();
	}
}
