#pragma once

#include "Common/FSecure/WinTools/UniqueHandle.h"

namespace FSecure::C3::Interfaces::Channels
{
	/// Implementation of a File Channel.
	struct MSSQL : public Channel<MSSQL>
	{

		/// Public constructor.
		/// @param arguments factory arguments.
		MSSQL(ByteView arguments);
		/// OnSend callback implementation. Called every time attached Relay wants to send a packet through this Channel Device. @see Device::OnSendToChannelInternal.
		/// @param packet data to send through the Channel.
		/// @return number of bytes successfully sent through the Channel. One call does not have to send all data. In that case chunking will take place, Chunks must be at least 64 bytes or equal to packet.size() to be accepted,
		size_t OnSendToChannel(FSecure::ByteView packet);

		/// Reads a single C3 packet from Channel. Periodically called by attached Relay. Implementation should read the data (or return an empty buffer if there's nothing in the Channel waiting to read) and leave as soon as possible.
		/// @return ByteVector that contains a single packet retrieved from Channel.
		std::vector<FSecure::ByteVector> OnReceiveFromChannel();

		/// Processes Commands addressed to this Channel.
		/// @param command a buffer containing whole command and it's parameters.
		/// @return command result.
		ByteVector OnRunCommand(ByteView command) override;

		/// Example Command handler. It must be registered/described in GetCapability. @see MyChannel::OnRunCommand.
		/// @returns ByteVector Command result.
		ByteVector ClearTable();

		/// Describes Channels creation parameters and custom Commands.
		/// @return Channel's capability description in JSON format.
		static const char* GetCapability();

		/// Explicit values used as the defaults for Channel's UpdateDelayJitter. Values can be changed later, at runtime.
		constexpr static std::chrono::milliseconds s_MinUpdateDelay = 1000ms, s_MaxUpdateDelay = 1000ms;

	private:
		/// The inbound direction name of data
		std::string m_InboundDirectionName;

		/// The outbound direction name, the opposite of m_inboundDirectionName
		std::string m_OutboundDirectionName;
		/// The server name to handle communication
		std::string m_ServerName;

		/// The database name to handle communication
		std::string m_DatabaseName;

		/// The table name to store all messages
		std::string m_TableName;

		/// The user to authenticate to the database
		std::string m_Username;

		/// The password for the user
		std::string m_Password;

		WinTools::UniqueHandle m_ImpersonationToken;

		bool m_UseSSPI = false;
	};
}
