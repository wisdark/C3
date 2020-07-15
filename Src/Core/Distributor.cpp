#include "StdAfx.h"
#include "Distributor.h"
#include "DeviceBridge.h"
#include "Common/FSecure/CppTools/ByteConverter/ByteConverter.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSecure::C3::Core::Distributor::Distributor(LoggerCallback callbackOnLog, Crypto::PrivateKey const& decryptionKey, Crypto::SymmetricKey const& broadcastKey)
	: m_CallbackOnLog{ callbackOnLog }
	, m_BroadcastKey{ broadcastKey }
	, m_DecryptionKey{ decryptionKey }
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSecure::C3::Core::Distributor::Log(LogMessage const& message, DeviceId sender) noexcept
{
	m_CallbackOnLog(message, std::string_view{ sender.IsNull() ? "" : sender.ToString() });
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSecure::C3::Core::Distributor::OnPacketReceived(ByteView packet, std::shared_ptr<DeviceBridge> sender)
{
	try
	{
		// Sanity check.
		if (packet.empty())
			throw std::runtime_error{ OBF("Received an empty packet.") };

		// Decrypt the packet and interpret the protocol.
		auto unlockedPacket = UnlockPacket(packet);
		switch (static_cast<Protocols>(unlockedPacket[0]))
		{
		case Protocols::N2N:
			return OnProtocolN2N(unlockedPacket, sender);

		case Protocols::S2G:
			return OnProtocolS2G(unlockedPacket, sender);

		case Protocols::G2A:
			return OnProtocolG2A(unlockedPacket, sender);

		case Protocols::G2R:
			return OnProtocolG2R(unlockedPacket, sender);

		default:
			throw std::runtime_error{ OBF("Unknown protocol: ") + std::to_string(unlockedPacket[0]) + OBF(".") };
		}
	}
	catch (std::runtime_error& e)
	{
		Log({ OBF_SEC("Packet handling failure. ") +e.what(), LogMessage::Severity::Error }, sender ? sender->GetDid() : DeviceId{});
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FSecure::C3::Core::Distributor::IsAgentBanned(AgentId agentId)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSecure::C3::Core::Distributor::OnProtocolN2N(ByteView packet0, std::shared_ptr<DeviceBridge> sender)
{
	// Protocol structure: [NeighborToNeighbor][SENDERS AID.IID][N2N Procedure][FIELDS]...
	try
	{
		// Sanity check.
		if (packet0.size() < 1 + RouteId::BinarySize + 1)
			throw std::invalid_argument{ OBF("N2N packet too short.") };

		packet0.remove_prefix(1);

		// Parse neighbor identifier and check whether is banned.
		auto neighborRouteId = packet0.Read<RouteId>();
		if (IsAgentBanned(neighborRouteId.GetAgentId()))
			return Log({ OBF("Received packet from a banned Agent ") + neighborRouteId.ToString() + OBF("."), LogMessage::Severity::Warning });

		// Handle Procedure part.
		return ProceduresN2N::RequestHandler::ParseRequestAndHandleIt(sender, neighborRouteId, packet0);
	}
	catch (std::exception & exception)
	{
		throw std::runtime_error{ OBF_STR("Failed to parse N2N packet. ") + exception.what() };
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FSecure::C3::Core::Distributor::LockAndSendPacket(ByteView packet, std::shared_ptr<DeviceBridge> channel)
{
	channel->OnPassNetworkPacket(FSecure::Crypto::EncryptAnonymously(packet, m_BroadcastKey));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FSecure::ByteVector FSecure::C3::Core::Distributor::UnlockPacket(ByteView packet)
{
	return FSecure::Crypto::DecryptFromAnonymous(packet, m_BroadcastKey);
}
