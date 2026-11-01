#include "app/Options.hpp"

#include <limits>
#include <thread>
#include <utility>

namespace apdfs
{

namespace
{

bool parseUnsigned(const std::string& text, uint64_t& out)
{
    if (text.empty())
    {
        return false;
    }

    uint64_t value = 0;
    for (const char c : text)
    {
        if (c < '0' || c > '9')
        {
            return false;
        }
        const auto digit = static_cast<uint64_t>(c - '0');
        if (value > (std::numeric_limits<uint64_t>::max() - digit) / 10)
        {
            return false;
        }
        value = value * 10 + digit;
    }

    out = value;
    return true;
}

bool splitOption(const std::string& arg, std::string& key, std::string& value)
{
    const size_t eq = arg.find('=');
    if (eq == std::string::npos)
    {
        return false;
    }
    key = arg.substr(2, eq - 2);
    value = arg.substr(eq + 1);
    return true;
}

bool isKnownKey(const std::string& key)
{
    return key == "threads" || key == "output" || key == "max-cuts" || key == "max-cut-size" || key == "max-examined";
}

} // namespace

std::string usageText()
{
    return "apdfs - enumerate every inclusion-minimal (S,T)-cut of a directed graph\n"
           "\n"
           "Usage:\n"
           "  apdfs <graph_file> <source> <sink> [threads] [output_dir] [options]\n"
           "\n"
           "Positional arguments:\n"
           "  graph_file        text graph: optional '# comments', then '<vertices> <edges>',\n"
           "                    then one '<u> <v>' line per edge\n"
           "  source, sink      vertex ids of S* and T* (0 .. 4294967295)\n"
           "  threads           worker threads (default: hardware concurrency)\n"
           "  output_dir        output directory (default: ./output)\n"
           "\n"
           "Options (override the positional arguments):\n"
           "  --threads=N       same as the positional threads argument\n"
           "  --output=DIR      same as the positional output_dir argument\n"
           "  --progress        print a live line with cuts, duplicates, memory and ETA\n"
           "  --max-cuts=N      stop after N cuts have been emitted (0 = no limit)\n"
           "  --max-cut-size=N  emit only cuts with at most N edges; this is an output\n"
           "                    filter, the search still visits larger cuts\n"
           "  --max-examined=N  work budget: stop after N cuts have been examined\n"
           "                    (emitted + size-filtered); 0 = no limit\n"
           "  -h, --help        print this text and exit\n"
           "  --version         print the version and exit\n"
           "\n"
           "Output:\n"
           "  cuts_<timestamp>_t<thread>_<index>.bin   binary cuts, 4 MiB per file\n"
           "  edges.map                                edge index -> 'source target'\n"
           "\n"
           "Exit status: 0 on success, 1 on runtime failure, 2 on invalid arguments.\n";
}

OptionsResult parseOptions(int argc, const char* const* argv)
{
    OptionsResult result;
    Options& options = result.options;

    const unsigned hw = std::thread::hardware_concurrency();
    options.threads = (hw == 0U) ? 1U : static_cast<size_t>(hw);
    options.outputDir = std::filesystem::current_path() / "output";

    std::vector<std::string> positional;
    std::vector<std::pair<std::string, std::string>> explicitOptions;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            options.help = true;
            continue;
        }
        if (arg == "--version")
        {
            options.version = true;
            continue;
        }
        if (arg == "--progress")
        {
            options.showProgress = true;
            continue;
        }
        if (!arg.starts_with("--"))
        {
            positional.push_back(arg);
            continue;
        }

        std::string key;
        std::string value;
        if (!splitOption(arg, key, value))
        {
            result.error = "option requires a value: " + arg;
            return result;
        }
        if (!isKnownKey(key))
        {
            result.error = "unknown option: --" + key;
            return result;
        }
        explicitOptions.emplace_back(std::move(key), std::move(value));
    }

    if (options.help || options.version)
    {
        return result;
    }

    if (positional.size() < 3)
    {
        result.error = "missing required arguments: <graph_file> <source> <sink>";
        return result;
    }
    if (positional.size() > 5)
    {
        result.error = "too many positional arguments";
        return result;
    }

    options.graphFile = positional[0];

    uint64_t source = 0;
    uint64_t sink = 0;
    if (!parseUnsigned(positional[1], source) || source > std::numeric_limits<VertexIndex>::max())
    {
        result.error = "invalid source: " + positional[1];
        return result;
    }
    if (!parseUnsigned(positional[2], sink) || sink > std::numeric_limits<VertexIndex>::max())
    {
        result.error = "invalid sink: " + positional[2];
        return result;
    }
    options.source = static_cast<VertexIndex>(source);
    options.sink = static_cast<VertexIndex>(sink);

    if (positional.size() >= 4)
    {
        uint64_t threads = 0;
        if (!parseUnsigned(positional[3], threads) || threads == 0)
        {
            result.error = "invalid thread count: " + positional[3];
            return result;
        }
        options.threads = static_cast<size_t>(threads);
    }
    if (positional.size() == 5)
    {
        options.outputDir = positional[4];
    }

    for (const auto& [key, value] : explicitOptions)
    {
        uint64_t number = 0;
        if (key == "threads")
        {
            if (!parseUnsigned(value, number) || number == 0)
            {
                result.error = "invalid value for --threads: " + value;
                return result;
            }
            options.threads = static_cast<size_t>(number);
        }
        else if (key == "output")
        {
            if (value.empty())
            {
                result.error = "empty value for --output";
                return result;
            }
            options.outputDir = value;
        }
        else if (key == "max-cuts")
        {
            if (!parseUnsigned(value, number))
            {
                result.error = "invalid value for --max-cuts: " + value;
                return result;
            }
            options.maxCuts = number;
        }
        else if (key == "max-cut-size")
        {
            if (!parseUnsigned(value, number))
            {
                result.error = "invalid value for --max-cut-size: " + value;
                return result;
            }
            options.maxCutSize = static_cast<size_t>(number);
        }
        else if (key == "max-examined")
        {
            if (!parseUnsigned(value, number))
            {
                result.error = "invalid value for --max-examined: " + value;
                return result;
            }
            options.maxExamined = number;
        }
        else
        {
            result.error = "unknown option: --" + key;
            return result;
        }
    }

    return result;
}

} // namespace apdfs
