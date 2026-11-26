/**
  * Copyright (c) 2026 DefKorns (https://defkorns.github.io/LICENSE)
  *
  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.
  */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

const int kMaxMenuDepth = 8;
const char kDefaultCoverHeader[] = "0000008c000000cc";

struct DirCloser {
    void operator()(DIR* dir) const { closedir(dir); }
};
using DirPtr = std::unique_ptr<DIR, DirCloser>;

class Fd {
public:
    explicit Fd(const std::string& path) : m_fd(open(path.c_str(), O_RDONLY)) {}
    ~Fd()
    {
        if (m_fd >= 0)
            close(m_fd);
    }
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    bool ok() const { return m_fd >= 0; }
    int get() const { return m_fd; }

private:
    int m_fd;
};

bool isFile(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool isDir(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::vector<std::string> listDir(const std::string& path)
{
    std::vector<std::string> names;
    DirPtr dir(opendir(path.c_str()));
    if (!dir)
        return names;
    while (const dirent* entry = readdir(dir.get())) {
        const std::string name = entry->d_name;
        if (name != "." && name != "..")
            names.push_back(name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool globMatch(const std::string& pattern, const std::string& text)
{
    size_t p = 0;
    size_t t = 0;
    size_t star = std::string::npos;
    size_t mark = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == text[t])) {
            ++p;
            ++t;
        } else if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            mark = t;
        } else if (star != std::string::npos) {
            p = star + 1;
            t = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;
    return p == pattern.size();
}

std::vector<std::string> globDir(const std::string& dir, const std::string& pattern)
{
    std::vector<std::string> matches;
    for (const std::string& name : listDir(dir))
        if (name[0] != '.' && globMatch(pattern, name))
            matches.push_back(name);
    return matches;
}

bool readFile(const std::string& path, std::string& content)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
        return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    content = buffer.str();
    return true;
}

bool writeFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    out << content;
    return static_cast<bool>(out);
}

void removeTree(const std::string& path)
{
    struct stat st;
    if (lstat(path.c_str(), &st) != 0)
        return;
    if (S_ISDIR(st.st_mode)) {
        for (const std::string& name : listDir(path))
            removeTree(path + "/" + name);
        rmdir(path.c_str());
    } else {
        unlink(path.c_str());
    }
}

std::vector<std::string> splitLines(const std::string& content)
{
    std::vector<std::string> lines;
    size_t start = 0;
    while (start < content.size()) {
        size_t end = content.find('\n', start);
        if (end == std::string::npos)
            end = content.size();
        std::string line = content.substr(start, end - start);
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        lines.push_back(line);
        start = end + 1;
    }
    return lines;
}

std::vector<std::string> splitWords(const std::string& text)
{
    std::vector<std::string> words;
    std::istringstream in(text);
    std::string word;
    while (in >> word)
        words.push_back(word);
    return words;
}

std::string lower(std::string text)
{
    for (char& c : text)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return text;
}

std::string baseName(const std::string& path)
{
    const size_t slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool startsWith(const std::string& text, const std::string& prefix)
{
    return text.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& text, const std::string& suffix)
{
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string xmlEscape(const std::string& text)
{
    std::string out;
    for (const char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        default: out += c;
        }
    }
    return out;
}

std::string replaceAll(std::string text, const std::string& from, const std::string& to)
{
    std::string out;
    size_t pos;
    while ((pos = text.find(from)) != std::string::npos) {
        out += text.substr(0, pos) + to;
        text = text.substr(pos + from.size());
    }
    return out + text;
}

std::string stripKey(const std::string& line, const std::string& key)
{
    if (!startsWith(line, key))
        return std::string();
    size_t pos = key.size();
    while (pos < line.size() && line[pos] == ' ')
        ++pos;
    if (pos >= line.size() || line[pos] != '=')
        return std::string();
    ++pos;
    while (pos < line.size() && line[pos] == ' ')
        ++pos;
    return line.substr(pos);
}

bool hasKey(const std::string& line, const std::string& key)
{
    if (!startsWith(line, key))
        return false;
    size_t pos = key.size();
    while (pos < line.size() && line[pos] == ' ')
        ++pos;
    return pos < line.size() && line[pos] == '=';
}

struct Desktop {
    std::string name;
    std::string exec;
    std::string icon;
    std::string release;
    std::string players;
    std::string genre;
    std::string description;
};

Desktop parseDesktop(const std::string& content, bool strictExec)
{
    Desktop d;
    std::string sortGenre;
    bool haveName = false;
    bool haveExec = false;
    bool haveIcon = false;
    bool haveRelease = false;
    bool havePlayers = false;
    bool haveGenre = false;
    bool haveSortGenre = false;
    bool inDescription = false;

    for (const std::string& line : splitLines(content)) {
        if (startsWith(line, "[Description]")) {
            inDescription = true;
            continue;
        }
        if (!line.empty() && line[0] == '[')
            inDescription = false;
        if (inDescription) {
            std::string text = hasKey(line, "Text") ? stripKey(line, "Text") : line;
            if (!text.empty())
                d.description += (d.description.empty() ? "" : " ") + text;
        }
        if (!haveName && hasKey(line, "Name")) {
            d.name = stripKey(line, "Name");
            haveName = true;
        } else if (!haveExec && (strictExec ? startsWith(line, "Exec=") : hasKey(line, "Exec"))) {
            d.exec = strictExec ? line.substr(5) : stripKey(line, "Exec");
            haveExec = true;
        } else if (!haveIcon && hasKey(line, "Icon")) {
            d.icon = stripKey(line, "Icon");
            haveIcon = true;
        } else if (!haveRelease && hasKey(line, "ReleaseDate")) {
            d.release = stripKey(line, "ReleaseDate");
            haveRelease = true;
        } else if (!havePlayers && hasKey(line, "Players")) {
            d.players = stripKey(line, "Players");
            havePlayers = true;
        } else if (!haveGenre && hasKey(line, "Genre")) {
            d.genre = stripKey(line, "Genre");
            haveGenre = true;
        } else if (!haveSortGenre && hasKey(line, "sortRawGenre")) {
            sortGenre = stripKey(line, "sortRawGenre");
            haveSortGenre = true;
        }
    }
    if (d.genre.empty())
        d.genre = sortGenre;
    return d;
}

bool readBytes(const std::string& path, uint64_t offset, size_t count, std::string& bytes)
{
    Fd fd(path);
    if (!fd.ok())
        return false;
    bytes.assign(count, '\0');
    const ssize_t got = pread(fd.get(), &bytes[0], count, static_cast<off_t>(offset));
    if (got != static_cast<ssize_t>(count))
        return false;
    return true;
}

bool readHex(const std::string& path, uint64_t offset, size_t count, std::string& hex)
{
    std::string bytes;
    if (!readBytes(path, offset, count, bytes))
        return false;
    static const char digits[] = "0123456789abcdef";
    hex.clear();
    for (const char c : bytes) {
        const unsigned char byte = static_cast<unsigned char>(c);
        hex += digits[byte >> 4];
        hex += digits[byte & 15];
    }
    return true;
}

bool readBigEndian(const std::string& path, uint64_t offset, size_t count, uint64_t& value)
{
    std::string bytes;
    if (!readBytes(path, offset, count, bytes))
        return false;
    value = 0;
    for (const char c : bytes)
        value = (value << 8) | static_cast<unsigned char>(c);
    return true;
}

struct Game {
    std::string menu;
    std::string code;
    std::string exe;
    std::string ext;
    std::string icon;
    std::string name;
    std::string description;
    std::string year;
    std::string players;
    std::string genre;
    std::string cover;
    bool isMenu = false;
    std::string menuTarget;
};

struct Context {
    std::string store;
    std::string out;
    std::string covers = "/media/data/emulationstation/covers";
    std::string squashfs = "/var/squashfs";
    std::map<std::string, std::string> romaji;
    std::set<std::string> defaultCovers;
};

std::string chdSystem(const Context& ctx, const std::string& code, const std::string& gameDir)
{
    std::string file;
    const std::string storageDir = ctx.store + "/.storage/" + code;
    for (const std::string& dir : { storageDir, gameDir }) {
        for (const std::string& name : globDir(dir, "*.chd")) {
            if (isFile(dir + "/" + name)) {
                file = dir + "/" + name;
                break;
            }
        }
        if (!file.empty())
            break;
    }
    if (file.empty())
        return std::string();

    std::string magic;
    if (!readHex(file, 0, 8, magic) || magic != "4d436f6d70724844")
        return std::string();

    uint64_t meta = 0;
    if (!readBigEndian(file, 48, 8, meta))
        return std::string();
    for (int steps = 0; meta > 0 && meta < (uint64_t(1) << 62) && steps < 32; ++steps) {
        std::string tag;
        if (!readHex(file, meta, 4, tag))
            break;
        if (tag == "44564420")
            return "psp";
        if (tag == "43485432" || tag == "43485452") {
            uint64_t length = 0;
            std::string track;
            if (readBigEndian(file, meta + 5, 3, length) && readBytes(file, meta + 16, static_cast<size_t>(length), track)) {
                track.erase(std::remove(track.begin(), track.end(), '\0'), track.end());
                if (track.find("MODE2") != std::string::npos)
                    return "psx";
                if (track.find("MODE1") != std::string::npos)
                    return "segacd";
            }
        }
        if (!readBigEndian(file, meta + 8, 8, meta))
            break;
    }
    return std::string();
}

bool oneOf(const std::string& value, std::initializer_list<const char*> patterns)
{
    for (const char* pattern : patterns)
        if (globMatch(pattern, value))
            return true;
    return false;
}

std::string systemOf(const Context& ctx, const Game& g, const std::string& gameDir)
{
    const std::string exe = startsWith(g.exe, "km_") ? g.exe.substr(3) : g.exe;
    const std::string& ext = g.ext;
    std::string system = "other";

    if (oneOf(exe, { "clover-canoe-shvc*", "snes", "snes05*", "snes9x*", "snes10*", "mednafen-snes-faust-xtreme", "mednafen_snes_faust_xtreme" }))
        system = "snes";
    else if (oneOf(exe, { "nestopia", "quicknes", "mesen", "fceumm*", "kachikachi*", "clover-kachikachi*", "nes" }))
        system = ext == "fds" ? "fds" : "nes";
    else if (oneOf(exe, { "gambatte", "gearboy", "tgbdual", "gb", "gbc" }))
        system = (ext == "gbc" || g.exe == "gbc") ? "gbc" : "gb";
    else if (oneOf(exe, { "mgba", "gpsp", "gba" }))
        system = "gba";
    else if (oneOf(exe, { "melonds", "desmume", "drastic*", "nds" }))
        system = "nds";
    else if (oneOf(exe, { "gg" }))
        system = "gamegear";
    else if (oneOf(exe, { "sms", "smsplus", "gearsystem" }))
        system = ext == "gg" ? "gamegear" : "mastersystem";
    else if (oneOf(exe, { "segacd" }))
        system = "segacd";
    else if (oneOf(exe, { "32x" }))
        system = "sega32x";
    else if (oneOf(exe, { "md" }))
        system = "megadrive";
    else if (oneOf(exe, { "genesis*", "picodrive*", "m2engage" })) {
        if (ext == "sms")
            system = "mastersystem";
        else if (ext == "gg")
            system = "gamegear";
        else if (ext == "32x")
            system = "sega32x";
        else if (ext == "sg")
            system = "sg-1000";
        else if (ext == "cue" || ext == "chd" || ext == "iso")
            system = "segacd";
        else
            system = "megadrive";
    } else if (oneOf(exe, { "mednafen-pce-fast", "mednafen_pce_fast", "pce", "pcecd", "mednafen-supergrafx", "mednafen_supergrafx", "sgfx" }))
        system = "pcengine";
    else if (oneOf(exe, { "mednafen-pcfx", "mednafen_pcfx", "pcfx" }))
        system = "pcfx";
    else if (oneOf(exe, { "mednafen-ngp", "mednafen_ngp", "ngp", "ngc" }))
        system = "ngp";
    else if (oneOf(exe, { "mednafen-wswan", "mednafen_wswan", "ws", "wsc" }))
        system = "wonderswan";
    else if (oneOf(exe, { "mednafen-vb", "mednafen_vb", "vb" }))
        system = "virtualboy";
    else if (oneOf(exe, { "mednafen-psx", "mednafen_psx", "pcsx*", "psx", "neon" }))
        system = "psx";
    else if (oneOf(exe, { "neocd", "neo" }))
        system = "neogeo";
    else if (oneOf(exe, { "fbalpha*", "fba*", "fbneo*", "cps1", "cps2", "cps3", "mame*" }))
        system = "arcade";
    else if (oneOf(exe, { "n64", "z64", "v64", "glupen64*", "mupen64*", "parallel-n64*", "parallel_n64*", "ludicrousn64*" }))
        system = "n64";
    else if (oneOf(exe, { "ppsspp*", "psp" }))
        system = "psp";
    else if (oneOf(exe, { "yabause*" }))
        system = "saturn";
    else if (oneOf(exe, { "morpheuscast*", "vemulator", "vmu" }))
        system = "dreamcast";
    else if (oneOf(exe, { "opera-xtreme", "opera_xtreme" }))
        system = "3do";
    else if (oneOf(exe, { "stella", "a26" }))
        system = "atari2600";
    else if (oneOf(exe, { "prosystem", "a78" }))
        system = "atari7800";
    else if (oneOf(exe, { "a52" }))
        system = "atari5200";
    else if (oneOf(exe, { "atari800" }))
        system = "atari800";
    else if (oneOf(exe, { "handy", "lnx" }))
        system = "atarilynx";
    else if (oneOf(exe, { "virtualjaguar", "j64" }))
        system = "atarijaguar";
    else if (oneOf(exe, { "hatari*" }))
        system = "atarist";
    else if (oneOf(exe, { "vice*", "c64", "c128", "vic20" }))
        system = "c64";
    else if (oneOf(exe, { "puae*", "uae4arm*", "amiga" }))
        system = "amiga";
    else if (oneOf(exe, { "col" }))
        system = "colecovision";
    else if (oneOf(exe, { "fmsx", "bluemsx", "msx" }))
        system = "msx";
    else if (oneOf(exe, { "caprice32", "cap32", "crocods" }))
        system = "amstradcpc";
    else if (oneOf(exe, { "fuse", "z80", "zx" }))
        system = "zxspectrum";
    else if (oneOf(exe, { "freeintv", "int" }))
        system = "intellivision";
    else if (oneOf(exe, { "freechaf" }))
        system = "channelf";
    else if (oneOf(exe, { "o2em", "odyssey" }))
        system = "videopac";
    else if (oneOf(exe, { "vecx", "vec" }))
        system = "vectrex";
    else if (oneOf(exe, { "gw", "mgw" }))
        system = "gameandwatch";
    else if (oneOf(exe, { "pokemini", "min", "pokemon" }))
        system = "pokemini";
    else if (oneOf(exe, { "dosbox*" }))
        system = "pc";
    else if (oneOf(exe, { "scummvm" }))
        system = "scummvm";
    else if (oneOf(exe, { "np2*", "nekop2", "pc98" }))
        system = "pc98";
    else if (oneOf(exe, { "px68k*", "sharp" }))
        system = "x68000";
    else if (oneOf(exe, { "quasi88", "d88" }))
        system = "pc88";
    else if (oneOf(exe, { "x1", "x1-millennium", "x1_millennium" }))
        system = "x1";
    else if (oneOf(exe, { "tic80", "tic" }))
        system = "tic80";
    else if (oneOf(exe, { "openbor*" }))
        system = "openbor";
    else if (oneOf(exe, { "easyrpg", "ldb", "chai*", "lutro", "lua", "prboom*", "wad", "nxengine", "cavestory", "ecwolf", "tyrquake", "pak", "xrick", "mrboom", "jumpnbump", "cannonball", "reminiscence", "flashback", "2048", "dinothawr", "bk", "eighty-one", "eighty_one", "dino", "81", "emux*", "ch8", "uzem", "uze", "potator" }))
        system = "ports";

    if (system == "other" && ext == "chd") {
        const std::string found = chdSystem(ctx, g.code, gameDir);
        if (!found.empty())
            system = found;
    }
    return system;
}

std::string romExtension(const std::string& rom)
{
    const size_t dot = rom.rfind('.');
    return dot == std::string::npos ? std::string() : rom.substr(dot + 1);
}

struct CoverIndex {
    bool present = false;
    std::set<std::string> exact;
    std::map<std::string, std::string> byLowerName;
    std::map<std::string, std::string> alias;
};

CoverIndex loadCovers(const std::string& dir)
{
    CoverIndex index;
    if (!isDir(dir))
        return index;
    index.present = true;
    for (const std::string& name : listDir(dir)) {
        index.exact.insert(name);
        const std::string lowered = lower(name);
        if (endsWith(lowered, ".png"))
            index.byLowerName.emplace(lowered, name);
    }
    std::string content;
    if (readFile(dir + "/aliases.txt", content)) {
        for (const std::string& line : splitLines(content)) {
            const size_t tab = line.find('\t');
            if (tab == std::string::npos)
                continue;
            std::string value = line.substr(tab + 1);
            const size_t second = value.find('\t');
            if (second != std::string::npos)
                value.erase(second);
            index.alias.emplace(line.substr(0, tab), value);
        }
    }
    return index;
}

std::string stripGroups(const std::string& text, char open, char close)
{
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == open) {
            const size_t end = text.find(close, i + 1);
            if (end != std::string::npos) {
                i = end;
                continue;
            }
        }
        out += text[i];
    }
    return out;
}

std::string findCover(const CoverIndex& index, const std::string& dir, const std::string& code, const std::string& rawName)
{
    if (!index.present)
        return std::string();
    if (index.exact.count(code + ".png"))
        return dir + "/" + code + ".png";

    const auto alias = index.alias.find(code);
    if (alias != index.alias.end() && !alias->second.empty()) {
        const auto hit = index.byLowerName.find(lower(alias->second + ".png"));
        if (hit != index.byLowerName.end())
            return dir + "/" + hit->second;
    }

    std::string key = lower(stripGroups(stripGroups(rawName, '(', ')'), '[', ']'));
    key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return !((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')); }), key.end());
    if (!key.empty()) {
        const auto hit = index.byLowerName.find(key + ".png");
        if (hit != index.byLowerName.end())
            return dir + "/" + hit->second;
    }
    return std::string();
}

std::set<std::string> findDefaultCovers(const std::string& store)
{
    std::set<std::string> codes;
    const std::string storage = store + "/.storage";
    auto check = [&codes](const std::string& path, const std::string& name) {
        if (!endsWith(name, ".png") || endsWith(name, "_small.png"))
            return;
        struct stat st;
        if (stat(path.c_str(), &st) != 0 || !S_ISREG(st.st_mode) || (st.st_size + 1023) / 1024 >= 8)
            return;
        std::string header;
        if (readHex(path, 16, 8, header) && header == kDefaultCoverHeader)
            codes.insert(name.substr(0, name.size() - 4));
    };
    for (const std::string& name : listDir(storage)) {
        const std::string path = storage + "/" + name;
        if (isDir(path)) {
            for (const std::string& inner : listDir(path))
                check(path + "/" + inner, inner);
        } else {
            check(path, name);
        }
    }
    return codes;
}

struct Output {
    std::string out;
    std::set<std::string> madeDirs;
    std::map<std::string, std::string> xml;
    std::vector<std::string> systemOrder;

    void ensureSystem(const std::string& system)
    {
        if (madeDirs.insert(system).second)
            mkdir((out + "/" + system).c_str(), 0755);
    }

    void appendXml(const std::string& system, const std::string& entry)
    {
        if (!xml.count(system))
            systemOrder.push_back(system);
        xml[system] += entry;
    }
};

std::string gameXml(const std::string& code, const std::string& name, const std::string& icon, const std::string& description,
    const std::string& year, const std::string& players, const std::string& genre, const std::string& small, const std::string& thumb)
{
    std::string xml = "  <game>\n";
    xml += "    <path>./" + code + ".desktop</path>\n";
    xml += "    <name>" + name + "</name>\n";
    if (!icon.empty())
        xml += "    <image>" + xmlEscape(icon) + "</image>\n";
    if (!description.empty())
        xml += "    <desc>" + description + "</desc>\n";
    if (year.size() == 8)
        xml += "    <releasedate>" + year + "T000000</releasedate>\n";
    if (!players.empty())
        xml += "    <players>" + players + "</players>\n";
    if (!genre.empty())
        xml += "    <genre>" + genre + "</genre>\n";
    xml += "    <code>" + code + "</code>\n";
    if (!small.empty())
        xml += "    <marquee>" + xmlEscape(small) + "</marquee>\n";
    if (!thumb.empty())
        xml += "    <thumbnail>" + xmlEscape(thumb) + "</thumbnail>\n";
    xml += "  </game>\n";
    return xml;
}

void walkMenu(const std::map<std::string, std::vector<Game>>& menus, const std::string& menu, int depth, std::set<std::string>& onPath, std::vector<const Game*>& result)
{
    onPath.insert(menu);
    const auto found = menus.find(menu);
    if (found != menus.end()) {
        for (const Game& game : found->second) {
            if (game.isMenu) {
                if (onPath.count(game.menuTarget) || depth >= kMaxMenuDepth)
                    continue;
                walkMenu(menus, game.menuTarget, depth + 1, onPath, result);
                continue;
            }
            result.push_back(&game);
        }
    }
    onPath.erase(menu);
}

std::map<std::string, std::string> loadRomaji(const std::string& file)
{
    std::map<std::string, std::string> table;
    std::string content;
    if (!readFile(file, content))
        return table;
    for (const std::string& line : splitLines(content)) {
        const size_t tab = line.find('\t');
        if (tab != std::string::npos)
            table[line.substr(0, tab)] = line.substr(tab + 1);
    }
    return table;
}

std::vector<const Game*> collectGames(const Context& ctx, const CoverIndex& covers, std::map<std::string, std::vector<Game>>& menus)
{
    for (const std::string& menu : globDir(ctx.store, "*")) {
        if (!isDir(ctx.store + "/" + menu))
            continue;
        for (const std::string& code : globDir(ctx.store + "/" + menu, "CLV-*")) {
            if (code == "CLV-Z-ESRET")
                continue;
            const std::string path = ctx.store + "/" + menu + "/" + code + "/" + code + ".desktop";
            std::string content;
            if (!isFile(path) || !readFile(path, content))
                continue;
            const Desktop d = parseDesktop(content, false);

            Game g;
            g.menu = menu;
            g.code = code;
            const auto rom = ctx.romaji.find(code);
            g.name = xmlEscape(rom != ctx.romaji.end() ? rom->second : d.name);
            g.icon = d.icon;
            g.description = xmlEscape(d.description);
            g.players = d.players;
            g.genre = xmlEscape(d.genre);

            const std::vector<std::string> words = splitWords(d.exec);
            g.exe = words.empty() ? std::string() : baseName(words[0]);
            std::string romName;
            for (size_t i = 1; i < words.size(); ++i) {
                if (words[i][0] != '-') {
                    romName = words[i];
                    break;
                }
            }
            romName = lower(baseName(romName));
            for (const char* suffix : { ".7z", ".zip", ".gz" }) {
                if (endsWith(romName, suffix)) {
                    romName.erase(romName.size() - std::string(suffix).size());
                    break;
                }
            }
            g.ext = romName.find('.') != std::string::npos ? romExtension(romName) : std::string();

            g.year = replaceAll(d.release, "-", "");
            g.cover = findCover(covers, ctx.covers, code, d.name);

            if (startsWith(d.exec, "/bin/chmenu ")) {
                g.isMenu = true;
                g.menuTarget = words.size() > 1 ? words[1] : std::string();
            }
            menus[menu].push_back(g);
        }
    }
    std::vector<const Game*> ordered;
    std::set<std::string> onPath;
    walkMenu(menus, "000", 0, onPath, ordered);
    return ordered;
}

void writeStoreGames(const Context& ctx, Output& output, const std::vector<const Game*>& games)
{
    const std::string smallCovers = ctx.covers + "_small";
    std::set<std::string> copied;
    for (const Game* game : games) {
        const Game& g = *game;
        const std::string gameDir = ctx.store + "/" + g.menu + "/" + g.code;
        const std::string system = systemOf(ctx, g, gameDir);
        if (!copied.insert(system + "/" + g.code).second)
            continue;
        output.ensureSystem(system);

        std::string chdTool;
        if (g.exe == "chd") {
            if (system == "psp")
                chdTool = "ppsspp";
            else if (system == "psx")
                chdTool = "psx";
            else if (system == "segacd")
                chdTool = "segacd";
        }
        std::string desktop;
        readFile(gameDir + "/" + g.code + ".desktop", desktop);
        if (!chdTool.empty()) {
            std::string rewritten;
            size_t start = 0;
            while (start < desktop.size()) {
                size_t end = desktop.find('\n', start);
                const bool hasNewline = end != std::string::npos;
                if (!hasNewline)
                    end = desktop.size();
                std::string line = desktop.substr(start, end - start);
                if (startsWith(line, "Exec=/bin/chd "))
                    line = "Exec=/bin/" + chdTool + " " + line.substr(14);
                rewritten += line;
                if (hasNewline)
                    rewritten += '\n';
                start = end + 1;
            }
            desktop = rewritten;
        }
        writeFile(output.out + "/" + system + "/" + g.code + ".desktop", desktop);

        std::string cover = g.cover;
        if (!cover.empty() && isFile(smallCovers + "/" + baseName(cover)))
            cover = smallCovers + "/" + baseName(cover);
        std::string icon = g.icon;
        if (startsWith(icon, "/var/games/"))
            icon = ctx.store + "/" + g.menu + "/" + icon.substr(11);
        if (!isFile(icon))
            icon.clear();
        if (!cover.empty())
            icon = cover;
        std::string small = gameDir + "/" + g.code + "_small.png";
        if (!isFile(small))
            small = icon;
        if (!cover.empty())
            small = cover;
        std::string thumb = gameDir + "/" + g.code + "_small.png";
        if (!isFile(thumb))
            thumb = small;
        if (ctx.defaultCovers.count(g.code) && cover.empty()) {
            icon.clear();
            small.clear();
            thumb.clear();
        }
        output.appendXml(system, gameXml(g.code, g.name, icon, g.description, g.year, g.players, g.genre, small, thumb));
    }
}

void writeOriginals(const Context& ctx, Output& output, bool stockOnly)
{
    const std::string root = ctx.squashfs + "/usr/share/games";
    std::vector<std::string> desktops;
    for (const std::string& first : globDir(root, "*")) {
        for (const std::string& second : globDir(root + "/" + first, "*")) {
            for (const std::string& code : globDir(root + "/" + first + "/" + second, "CLV-*"))
                desktops.push_back(root + "/" + first + "/" + second + "/" + code);
        }
    }
    for (const std::string& code : globDir(root, "CLV-*"))
        desktops.push_back(root + "/" + code);

    std::string codes;
    for (const std::string& gameDir : desktops) {
        const std::string code = baseName(gameDir);
        const std::string desktopPath = gameDir + "/" + code + ".desktop";
        if (!isFile(desktopPath))
            continue;
        if (!stockOnly) {
            codes += code + "\n";
            continue;
        }
        std::string content;
        readFile(desktopPath, content);
        const Desktop d = parseDesktop(content, true);

        std::string execLine = d.exec;
        const size_t space = execLine.find(' ');
        const std::string tool = space == std::string::npos ? execLine : execLine.substr(0, space);
        const std::string wrapper = "/bin/" + baseName(tool) + "-wr";
        if (access(wrapper.c_str(), X_OK) == 0)
            execLine = wrapper + " " + (space == std::string::npos ? execLine : execLine.substr(space + 1));
        execLine = replaceAll(replaceAll(execLine, "/var/squashfs/usr/share/games/", "/usr/share/games/"), "/usr/share/games/", "/var/squashfs/usr/share/games/");

        const std::vector<std::string> words = splitWords(execLine);
        std::string rom;
        for (size_t i = 1; i < words.size(); ++i) {
            if (words[i][0] != '-') {
                rom = lower(baseName(words[i]));
                break;
            }
        }
        for (const char* suffix : { ".7z", ".zip", ".gz" })
            if (endsWith(rom, suffix))
                rom.erase(rom.size() - std::string(suffix).size());

        Game g;
        g.code = code;
        g.exe = baseName(tool);
        g.ext = rom.find('.') != std::string::npos ? romExtension(rom) : std::string();
        const std::string system = systemOf(ctx, g, gameDir);
        output.ensureSystem(system);

        const std::string target = output.out + "/" + system + "/" + code + ".desktop";
        if (isFile(target))
            continue;
        std::string rewritten;
        for (const std::string& line : splitLines(content)) {
            if (startsWith(line, "Exec="))
                rewritten += "Exec=" + execLine + "\n";
            else if (startsWith(line, "Path="))
                rewritten += "Path=/var/saves/" + code + "\n";
            else
                rewritten += line + "\n";
        }
        writeFile(target, rewritten);

        std::string icon = gameDir + "/" + code + ".png";
        if (!isFile(icon))
            icon.clear();
        std::string small = gameDir + "/" + code + "_small.png";
        if (!isFile(small))
            small = icon;
        output.appendXml(system, gameXml(code, xmlEscape(d.name), icon, xmlEscape(d.description), replaceAll(d.release, "-", ""),
                                  d.players, xmlEscape(d.genre), small, small));
    }
    writeFile(output.out + "/originals.txt", codes);
}

std::string directoryOf(const std::string& program)
{
    const size_t slash = program.rfind('/');
    return slash == std::string::npos ? std::string(".") : program.substr(0, slash);
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2 || std::string(argv[1]).size() < 2) {
        std::cerr << "usage: es-library <output dir>\n";
        return 2;
    }

    if (std::string(argv[1]) == "--system-of") {
        Game game;
        game.exe = argc > 2 ? argv[2] : "";
        game.ext = argc > 3 ? argv[3] : "";
        std::cout << systemOf(Context(), game, std::string()) << "\n";
        return 0;
    }

    Context ctx;
    ctx.out = argv[1];
    if (const char* store = getenv("ES_STORE"))
        ctx.store = store;
    if (const char* squashfs = getenv("squashfs"))
        if (*squashfs)
            ctx.squashfs = squashfs;

    const std::string here = directoryOf(argv[0]);
    if (isFile(here + "/romaji_on"))
        ctx.romaji = loadRomaji(here + "/es_romaji.txt");

    removeTree(ctx.out);
    mkdir(ctx.out.c_str(), 0755);

    Output output;
    output.out = ctx.out;

    if (!ctx.store.empty()) {
        ctx.defaultCovers = findDefaultCovers(ctx.store);
        const CoverIndex covers = loadCovers(ctx.covers);
        std::map<std::string, std::vector<Game>> menus;
        writeStoreGames(ctx, output, collectGames(ctx, covers, menus));
    }
    writeOriginals(ctx, output, output.xml.empty());

    for (const std::string& system : output.systemOrder) {
        writeFile(output.out + "/" + system + "/gamelist.xml",
            "<?xml version=\"1.0\"?>\n<gameList>\n" + output.xml[system] + "</gameList>\n");
    }
    return 0;
}
