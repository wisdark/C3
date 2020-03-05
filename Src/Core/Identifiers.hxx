#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<typename UnderlyingIntegerType>
const FSecure::C3::Identifier<UnderlyingIntegerType> FSecure::C3::Identifier<UnderlyingIntegerType>::Null{ static_cast<UnderlyingIntegerType>(0) };

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
constexpr FSecure::C3::Identifier<UnderlyingIntegerType>::Identifier()
	: m_Id(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
constexpr FSecure::C3::Identifier<UnderlyingIntegerType>::Identifier(UnderlyingIntegerType id)
	: m_Id(id)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
FSecure::C3::Identifier<UnderlyingIntegerType>::Identifier(std::string_view textId)
{
	// Sanity check.
	//if (textId.size() != TextSize)
	//	throw std::runtime_error{ OBF("Invalid string Identifier size.") };

	// Parse and return.
	std::size_t pos;
	m_Id = static_cast<UnderlyingIntegerType>(std::stoull(textId.data(), &pos, 16));
	if (pos != textId.size())
		throw std::runtime_error{ OBF("Invalid Identifier string characters.") };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
FSecure::C3::Identifier<UnderlyingIntegerType>::Identifier(std::string const& textId)
	: Identifier{ std::string_view{textId} }
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
FSecure::C3::Identifier<UnderlyingIntegerType>::Identifier(ByteView byteId)
{
	// Sanity check.
	if (byteId.size() != BinarySize)
		throw std::runtime_error{ OBF("Invalid byte Identifier size.") };

	// Just make a byte-to-byte copy.
	memcpy(&m_Id, byteId.data(), sizeof(m_Id));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
FSecure::C3::Identifier<UnderlyingIntegerType> FSecure::C3::Identifier<UnderlyingIntegerType>::GenerateRandom()
{
	return FSecure::Utils::GenerateRandomValue<UnderlyingIntegerType>();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
std::string FSecure::C3::Identifier<UnderlyingIntegerType>::ToString() const
{
	// Initialize buffers and pointers.
	std::string ret(sizeof UnderlyingIntegerType * 2 + 1, '0');
	char* rp = ret.data();

	// Note: this function adds a null terminator.
	auto p = reinterpret_cast<const uint8_t*>(&m_Id);
	for (int i = 0; i < sizeof(UnderlyingIntegerType); ++i)
		sprintf_s(&rp[i * 2], 3, OBF("%02hhX"), p[sizeof(UnderlyingIntegerType) - i - 1]);

	// Remove the trailing null.
	return ret.substr(0, sizeof UnderlyingIntegerType * 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
FSecure::ByteVector FSecure::C3::Identifier<UnderlyingIntegerType>::ToByteVector() const
{
	return { reinterpret_cast<const std::uint8_t*>(&m_Id), reinterpret_cast<const std::uint8_t*>(&m_Id) + sizeof(m_Id) };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
bool FSecure::C3::Identifier<UnderlyingIntegerType>::operator!() const
{
	return IsNull();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
bool FSecure::C3::Identifier<UnderlyingIntegerType>::operator==(Identifier const& c) const
{
	return c.m_Id == m_Id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
bool FSecure::C3::Identifier<UnderlyingIntegerType>::operator!=(Identifier const& c) const
{
	return !operator == (c);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
bool FSecure::C3::Identifier<UnderlyingIntegerType>::operator<(Identifier const& c) const
{
	return m_Id < c.m_Id;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename UnderlyingIntegerType>
bool FSecure::C3::Identifier<UnderlyingIntegerType>::IsNull() const
{
	return m_Id == 0;
}

template<typename UnderlyingIntegerType>
UnderlyingIntegerType FSecure::C3::Identifier<UnderlyingIntegerType>::ToUnderlyingType() const
{
	return m_Id;
}
