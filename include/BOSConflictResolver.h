#pragma once

#include "ClibUtil/singleton.hpp"

class BOSConflictResolver : public clib_util::singleton::ISingleton<BOSConflictResolver> {
public:
    struct Entry {
        std::string lhs;   // left-hand side (formID~plugin)
        std::string rhs;   // right-hand side (replacement or transform)
        std::string file;  // which _SWAP.ini it came from
        int line;          // line number in file
    };

    // Maps: section -> LHS -> all conflicting entries
    using SectionConflicts = std::unordered_map<std::string,  // section name: "References", "Forms", "Transforms"
                                                std::unordered_map<std::string,        // lhs key
                                                                   std::vector<Entry>  // conflicting entries
                                                                   > >;

    BOSConflictResolver() = default;

    void Scan(const std::string& dataPath = "Data");
    const SectionConflicts& GetConflicts() const { return conflicts; }

    // Optionally: comment out specific entries in-place
    bool CommentOut(const Entry& entry);
    void Inspect(const Entry& entry);
    void Apply(const Entry& entry);

private:
    SectionConflicts conflicts;

    void ProcessFile(const std::filesystem::path& path);
    void ParseLine(const std::string& section, const std::string& line, const std::string& file, int lineNum);
};
