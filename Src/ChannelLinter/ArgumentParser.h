#pragma once

namespace FSecure::C3::Linter
{
	/// Channel linter command line arguments parser
	class ArgumentParser
	{
	public:
		/// Parse and validate arguments
		/// @param argc - number of arguments
		/// @param argv - vector of arguments
		/// @throws std::invalid_argument if config is not valid
		ArgumentParser(int argc, char** argv);

		/// @returns valid application config
		AppConfig const& GetConfig() const;

		/// @returns formatted usage string
		std::string GetUsage() const;

	private:
		/// Helper to configure internal parser
		void ConfigureParser();

		/// Helper create AppConfig
		void FillConfig();

		/// Validate created config, uses ArgumentParser options in messages
		/// @throws std::invalid_argument if config is not valid
		void ValidateConfig() const;

		/// place to store argv[0]
		std::filesystem::path m_AppName;

		/// Internal argument parser
		argparse::ArgumentParser m_ArgParser;

		/// Holds an application config, once arguments
		AppConfig m_Config;
	};
}

