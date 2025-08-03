#include <unordered_set>
#include <string>
#include <functional>

#include <time/measure_time.h>

class DedupeChecker
{
public:
    static bool is_duplicate(const std::string& msg)
    {
        static std::unordered_set<size_t> seen_hashes;

        // MeasureTime t("Check is_duplicate", MeasureUnit::MICROSECOND);

        size_t hash = std::hash<std::string>{}(msg);
        auto [it, inserted] = seen_hashes.insert(hash);
        return !inserted; // true if duplicate
    }
};