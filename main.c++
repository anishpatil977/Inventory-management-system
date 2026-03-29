// Feature-rich CLI pricing & inventory system
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <numeric>

using Clock = std::chrono::system_clock;

// ANSI color codes for better CLI output
namespace Color
{
    constexpr const char *GREEN = "\033[32m";
    constexpr const char *RED = "\033[31m";
    constexpr const char *YELLOW = "\033[33m";
    constexpr const char *BLUE = "\033[34m";
    constexpr const char *CYAN = "\033[36m";
    constexpr const char *RESET = "\033[0m";
    constexpr const char *BOLD = "\033[1m";
}

// Product structure with enhanced functionality
struct Product
{
    int id = 0;
    std::string name;
    double basePrice = 0.0;
    double taxRate = 0.0;      // 0..1
    double discountRate = 0.0; // 0..1
    std::string dateAdded;

    Product() = default;
    Product(int id_, std::string name_, double basePrice_, double taxRate_, double discountRate_, std::string dateAdded_ = "")
        : id(id_), name(std::move(name_)), basePrice(basePrice_), taxRate(taxRate_), discountRate(discountRate_), dateAdded(std::move(dateAdded_)) {}

    std::string toCSV() const
    {
        std::ostringstream os;
        std::string nm = name;
        size_t pos = 0;
        while ((pos = nm.find('"', pos)) != std::string::npos)
        {
            nm.insert(pos, "\"");
            pos += 2;
        }
        os << id << ',"' << nm << '",' << basePrice << ',' << taxRate << ',' << discountRate << ',' << dateAdded;
        return os.str();
    }

    static bool fromCSV(const std::string &line, Product &out)
    {
        std::istringstream is(line);
        std::string idStr;
        if (!std::getline(is, idStr, ','))
            return false;
        std::string namePart;
        if (!std::getline(is, namePart, '"'))
            return false;
        if (!std::getline(is, namePart, '"'))
            return false;
        std::string rest;
        if (!std::getline(is, rest))
            return false;
        if (!rest.empty() && rest.front() == ',')
            rest.erase(0, 1);
        std::istringstream rs(rest);
        double base = 0, tax = 0, disc = 0;
        char comma;
        std::string dateAdded;
        rs >> base >> comma >> tax >> comma >> disc;
        if (rs.peek() == ',')
        {
            rs.get();
            std::getline(rs, dateAdded);
        }
        try
        {
            int id = std::stoi(idStr);
            out = Product(id, namePart, base, tax, disc, dateAdded);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
};

// Pricing engine with validation
struct PricingEngine
{
    static double netPrice(double basePrice, double discountRate)
    {
        if (basePrice < 0.0)
            throw std::invalid_argument("Base price must be >= 0");
        if (discountRate < 0.0 || discountRate > 1.0)
            throw std::invalid_argument("Discount rate must be between 0..1");
        return basePrice * (1.0 - discountRate);
    }

    static double finalPrice(double basePrice, double discountRate, double taxRate)
    {
        double net = netPrice(basePrice, discountRate);
        if (taxRate < 0.0 || taxRate > 1.0)
            throw std::invalid_argument("Tax rate must be between 0..1");
        return net * (1.0 + taxRate);
    }

    static double totalInventoryValue(const std::vector<Product> &products)
    {
        return std::accumulate(products.begin(), products.end(), 0.0,
                               [](double sum, const Product &p)
                               {
                                   return sum + finalPrice(p.basePrice, p.discountRate, p.taxRate);
                               });
    }
};

// Inventory management system with enhanced features
class Inventory
{
private:
    std::map<int, Product> products_;
    std::string storagePath_;
    bool autosave_ = true;

public:
    explicit Inventory(std::string path = "inventory.csv") : storagePath_(std::move(path)) {}

    void setAutosave(bool v) noexcept { autosave_ = v; }

    bool add(Product p)
    {
        if (products_.count(p.id))
            return false;
        products_.emplace(p.id, std::move(p));
        if (autosave_)
            save();
        return true;
    }

    bool update(int id, const Product &p)
    {
        auto it = products_.find(id);
        if (it == products_.end())
            return false;
        it->second = p;
        if (autosave_)
            save();
        return true;
    }

    bool remove(int id)
    {
        auto removed = products_.erase(id) > 0;
        if (removed && autosave_)
            save();
        return removed;
    }

    const Product *find(int id) const
    {
        auto it = products_.find(id);
        if (it == products_.end())
            return nullptr;
        return &it->second;
    }

    std::vector<Product> list() const
    {
        std::vector<Product> out;
        out.reserve(products_.size());
        for (auto const &kv : products_)
            out.push_back(kv.second);
        return out;
    }

    std::vector<Product> searchByName(const std::string &q) const
    {
        std::vector<Product> out;
        std::string lowerQ = q;
        std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(), ::tolower);
        for (auto const &kv : products_)
        {
            std::string name = kv.second.name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(lowerQ) != std::string::npos)
                out.push_back(kv.second);
        }
        return out;
    }

    size_t size() const { return products_.size(); }

    void save(std::string path = "") const
    {
        std::string p = path.empty() ? storagePath_ : path;
        std::ofstream os(p, std::ios::trunc);
        if (!os)
            throw std::runtime_error("Cannot open file for save: " + p);
        for (auto const &kv : products_)
            os << kv.second.toCSV() << "\n";
    }

    void load(std::string path = "")
    {
        std::string p = path.empty() ? storagePath_ : path;
        products_.clear();
        std::ifstream is(p);
        if (!is)
            return;
        std::string line;
        while (std::getline(is, line))
        {
            Product p;
            if (Product::fromCSV(line, p))
                products_.emplace(p.id, p);
        }
    }

    void importCSV(const std::string &path)
    {
        std::ifstream is(path);
        if (!is)
            throw std::runtime_error("Cannot open import file: " + path);
        std::string line;
        while (std::getline(is, line))
        {
            Product p;
            if (Product::fromCSV(line, p))
                products_[p.id] = p;
        }
        if (autosave_)
            save();
    }
};

// Utility functions for time, logging, and formatting
static std::string nowStr()
{
    auto t = Clock::to_time_t(Clock::now());
    std::tm tm{};
#if defined(_MSC_VER)
    localtime_s(&tm, &t);
#elif defined(__unix__) || defined(__APPLE__)
    localtime_r(&t, &tm);
#else
    std::tm *ptm = std::localtime(&t);
    if (ptm)
        tm = *ptm;
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return os.str();
}

static void logLine(const std::string &fn, const std::string &line)
{
    std::ofstream os(fn, std::ios::app);
    if (!os)
        return;
    os << "[" << nowStr() << "] " << line << "\n";
}

static std::string formatMoney(double v)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(2) << "$" << v;
    return os.str();
}

static std::string formatPercent(double v)
{
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << (v * 100) << "%";
    return os.str();
}

// Helper for parsing numeric input with validation
static bool parseDouble(std::istringstream &is, double &out, const std::string &fieldName)
{
    if (!(is >> out))
    {
        std::cerr << Color::RED << "✗ Invalid " << fieldName << ": expected number" << Color::RESET << "\n";
        return false;
    }
    return true;
}

static bool parseInteger(std::istringstream &is, int &out, const std::string &fieldName)
{
    if (!(is >> out))
    {
        std::cerr << Color::RED << "✗ Invalid " << fieldName << ": expected integer" << Color::RESET << "\n";
        return false;
    }
    return true;
}

// Display functions with enhanced formatting
static void printHeader()
{
    std::cout << "\n"
              << Color::BOLD << Color::CYAN;
    std::cout << std::setw(8) << "ID"
              << std::setw(28) << "Name"
              << std::setw(14) << "Base Price"
              << std::setw(12) << "Discount"
              << std::setw(10) << "Tax"
              << std::setw(14) << "Final Price" << Color::RESET << "\n";
    std::cout << std::string(90, '-') << "\n";
}

static void printProduct(const Product &p)
{
    double final = PricingEngine::finalPrice(p.basePrice, p.discountRate, p.taxRate);
    std::cout << std::setw(8) << p.id
              << std::setw(28) << p.name.substr(0, 26)
              << std::setw(14) << formatMoney(p.basePrice)
              << std::setw(12) << formatPercent(p.discountRate)
              << std::setw(10) << formatPercent(p.taxRate)
              << Color::GREEN << std::setw(14) << formatMoney(final) << Color::RESET << "\n";
}

static void printStats(const std::vector<Product> &products)
{
    if (products.empty())
    {
        std::cout << Color::YELLOW << "  No products in inventory\n"
                  << Color::RESET;
        return;
    }

    double totalValue = PricingEngine::totalInventoryValue(products);
    double avgPrice = totalValue / products.size();
    double maxPrice = 0, minPrice = std::numeric_limits<double>::max();

    for (const auto &p : products)
    {
        double final = PricingEngine::finalPrice(p.basePrice, p.discountRate, p.taxRate);
        maxPrice = std::max(maxPrice, final);
        minPrice = std::min(minPrice, final);
    }

    std::cout << Color::BOLD << "\n📊 Inventory Statistics:\n"
              << Color::RESET;
    std::cout << "  Total Products:  " << Color::BLUE << products.size() << Color::RESET << "\n";
    std::cout << "  Total Value:     " << Color::GREEN << formatMoney(totalValue) << Color::RESET << "\n";
    std::cout << "  Average Price:   " << formatMoney(avgPrice) << "\n";
    std::cout << "  Highest Price:   " << Color::GREEN << formatMoney(maxPrice) << Color::RESET << "\n";
    std::cout << "  Lowest Price:    " << Color::YELLOW << formatMoney(minPrice) << Color::RESET << "\n";
}

static void showHelp()
{
    std::cout << Color::BOLD << Color::CYAN << "\n═══ INVENTORY CLI COMMANDS ═══\n"
              << Color::RESET;
    std::cout << Color::BOLD << "Basic Commands:\n"
              << Color::RESET;
    std::cout << "  help                 Show this help message\n";
    std::cout << "  list                 List all products with statistics\n";
    std::cout << "  search <query>       Search products by name\n\n";

    std::cout << Color::BOLD << "Product Management:\n"
              << Color::RESET;
    std::cout << "  add <id> <name> <base> <tax> <discount>\n";
    std::cout << "                       Add a new product (tax,discount in 0..1)\n";
    std::cout << "  update <id> <name> <base> <tax> <discount>\n";
    std::cout << "                       Update existing product\n";
    std::cout << "  remove <id>          Remove a product\n";
    std::cout << "  show <id>            Display single product details\n\n";

    std::cout << Color::BOLD << "File Operations:\n"
              << Color::RESET;
    std::cout << "  save [path]          Save inventory to CSV\n";
    std::cout << "  load [path]          Load inventory from CSV (clears existing)\n";
    std::cout << "  import <path>        Import CSV (merges with existing data)\n\n";

    std::cout << Color::BOLD << "Settings:\n"
              << Color::RESET;
    std::cout << "  autosave on|off      Enable/disable automatic saving\n";
    std::cout << "  exit                 Exit the program\n";
    std::cout << Color::BLUE << "\nℹ Use quotes for product names with spaces: \"Laptop Pro\"\n"
              << Color::RESET << "\n";
}

// Helper to parse product input from command line
static bool parseProductInput(std::istringstream &is, int &id, std::string &name, double &base, double &tax, double &discount, const std::string &cmdName)
{
    if (!parseInteger(is, id, "product ID"))
        return false;

    if (!(is >> std::ws) || is.peek() == EOF)
    {
        std::cerr << Color::RED << "✗ Missing product name\n"
                  << Color::RESET;
        return false;
    }

    if (is.peek() == '"')
    {
        is.get();
        std::getline(is, name, '"');
    }
    else
    {
        is >> name;
    }

    if (!parseDouble(is, base, "base price"))
        return false;
    if (!parseDouble(is, tax, "tax rate"))
        return false;
    if (!parseDouble(is, discount, "discount rate"))
        return false;

    // Validate ranges
    if (base < 0)
    {
        std::cerr << Color::RED << "✗ Base price cannot be negative\n"
                  << Color::RESET;
        return false;
    }
    if (tax < 0 || tax > 1)
    {
        std::cerr << Color::RED << "✗ Tax rate must be between 0 and 1\n"
                  << Color::RESET;
        return false;
    }
    if (discount < 0 || discount > 1)
    {
        std::cerr << Color::RED << "✗ Discount rate must be between 0 and 1\n"
                  << Color::RESET;
        return false;
    }

    return true;
}

int main()
{
    try
    {
        Inventory store("inventory.csv");
        store.setAutosave(true);
        store.load();
        const std::string logFile = "inventory.log";
        logLine(logFile, "=== Started inventory CLI ===");

        std::cout << Color::BOLD << Color::CYAN << "╔════════════════════════════════════════════╗\n"
                  << "║   📦 Inventory & Pricing System (C++)     ║\n"
                  << "║   Type 'help' for available commands      ║\n"
                  << "╚════════════════════════════════════════════╝" << Color::RESET << "\n\n";

        std::string line;
        while (true)
        {
            std::cout << Color::BOLD << Color::BLUE << "> " << Color::RESET;
            if (!std::getline(std::cin, line))
                break;

            std::istringstream is(line);
            std::string cmd;
            if (!(is >> cmd))
                continue;

            try
            {
                if (cmd == "help")
                {
                    showHelp();
                    continue;
                }

                if (cmd == "add")
                {
                    int id;
                    std::string name;
                    double base, tax, disc;

                    if (!parseProductInput(is, id, name, base, tax, disc, "add"))
                        continue;

                    Product p{id, name, base, tax, disc, nowStr()};
                    if (!store.add(p))
                    {
                        std::cerr << Color::RED << "✗ Product ID " << id << " already exists\n"
                                  << Color::RESET;
                        continue;
                    }
                    logLine(logFile, "Added product ID=" + std::to_string(id) + " name=" + name);
                    std::cout << Color::GREEN << "✓ Product added successfully\n"
                              << Color::RESET;
                    continue;
                }

                if (cmd == "update")
                {
                    int id;
                    std::string name;
                    double base, tax, disc;

                    if (!parseProductInput(is, id, name, base, tax, disc, "update"))
                        continue;

                    Product p{id, name, base, tax, disc, nowStr()};
                    if (!store.update(id, p))
                    {
                        std::cerr << Color::RED << "✗ Product ID " << id << " not found\n"
                                  << Color::RESET;
                        continue;
                    }
                    logLine(logFile, "Updated product ID=" + std::to_string(id));
                    std::cout << Color::GREEN << "✓ Product updated successfully\n"
                              << Color::RESET;
                    continue;
                }

                if (cmd == "remove")
                {
                    int id;
                    if (!parseInteger(is, id, "product ID"))
                        continue;

                    if (!store.remove(id))
                    {
                        std::cerr << Color::RED << "✗ Product ID " << id << " not found\n"
                                  << Color::RESET;
                        continue;
                    }
                    logLine(logFile, "Removed product ID=" + std::to_string(id));
                    std::cout << Color::GREEN << "✓ Product removed successfully\n"
                              << Color::RESET;
                    continue;
                }

                if (cmd == "show")
                {
                    int id;
                    if (!parseInteger(is, id, "product ID"))
                        continue;

                    auto op = store.find(id);
                    if (!op)
                    {
                        std::cerr << Color::RED << "✗ Product ID " << id << " not found\n"
                                  << Color::RESET;
                        continue;
                    }
                    printHeader();
                    printProduct(*op);
                    continue;
                }

                if (cmd == "list")
                {
                    auto items = store.list();
                    printHeader();
                    for (auto const &p : items)
                        printProduct(p);
                    printStats(items);
                    std::cout << "\n";
                    continue;
                }

                if (cmd == "search")
                {
                    std::string q;
                    if (!(is >> std::ws) || is.peek() == EOF)
                    {
                        std::cerr << Color::RED << "✗ Missing search query\n"
                                  << Color::RESET;
                        continue;
                    }

                    if (is.peek() == '"')
                    {
                        is.get();
                        std::getline(is, q, '"');
                    }
                    else
                    {
                        is >> q;
                    }

                    auto res = store.searchByName(q);
                    if (res.empty())
                    {
                        std::cout << Color::YELLOW << "  No products found matching '" << q << "'\n"
                                  << Color::RESET;
                        continue;
                    }
                    printHeader();
                    for (auto const &p : res)
                        printProduct(p);
                    printStats(res);
                    std::cout << "\n";
                    continue;
                }

                if (cmd == "save")
                {
                    std::string path;
                    if (is >> path)
                    {
                        store.save(path);
                        std::cout << Color::GREEN << "✓ Inventory saved to " << path << "\n"
                                  << Color::RESET;
                    }
                    else
                    {
                        store.save();
                        std::cout << Color::GREEN << "✓ Inventory saved to default location\n"
                                  << Color::RESET;
                    }
                    logLine(logFile, "Saved inventory");
                    continue;
                }

                if (cmd == "load")
                {
                    std::string path;
                    if (is >> path)
                    {
                        store.load(path);
                        std::cout << Color::GREEN << "✓ Inventory loaded from " << path << "\n"
                                  << Color::RESET;
                        logLine(logFile, "Loaded inventory from " + path);
                    }
                    else
                    {
                        store.load();
                        std::cout << Color::GREEN << "✓ Inventory loaded from default location\n"
                                  << Color::RESET;
                        logLine(logFile, "Loaded inventory");
                    }
                    continue;
                }

                if (cmd == "import")
                {
                    std::string path;
                    if (!(is >> path))
                    {
                        std::cerr << Color::RED << "✗ Missing file path\n"
                                  << Color::RESET;
                        continue;
                    }
                    store.importCSV(path);
                    std::cout << Color::GREEN << "✓ Inventory imported from " << path << "\n"
                              << Color::RESET;
                    logLine(logFile, "Imported inventory from " + path);
                    continue;
                }

                if (cmd == "autosave")
                {
                    std::string val;
                    if (!(is >> val))
                    {
                        std::cerr << Color::RED << "✗ Missing argument: use 'on' or 'off'\n"
                                  << Color::RESET;
                        continue;
                    }
                    if (val == "on")
                    {
                        store.setAutosave(true);
                        std::cout << Color::GREEN << "✓ Autosave enabled\n"
                                  << Color::RESET;
                    }
                    else if (val == "off")
                    {
                        store.setAutosave(false);
                        std::cout << Color::YELLOW << "⚠ Autosave disabled\n"
                                  << Color::RESET;
                    }
                    else
                    {
                        std::cerr << Color::RED << "✗ Invalid value: use 'on' or 'off'\n"
                                  << Color::RESET;
                    }
                    continue;
                }

                if (cmd == "exit" || cmd == "quit")
                {
                    std::cout << Color::CYAN << "Goodbye! 👋\n"
                              << Color::RESET;
                    break;
                }

                std::cerr << Color::RED << "✗ Unknown command: '" << cmd << "' (type 'help' for commands)\n"
                          << Color::RESET;
            }
            catch (const std::exception &ex)
            {
                std::cerr << Color::RED << "✗ Error: " << ex.what() << Color::RESET << "\n";
                logLine(logFile, "Error: " + std::string(ex.what()));
            }
        }

        logLine(logFile, "=== Exiting inventory CLI ===");
    }
    catch (const std::exception &ex)
    {
        std::cerr << Color::RED << Color::BOLD << "FATAL ERROR: " << ex.what() << Color::RESET << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}