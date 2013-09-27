/***************************************************************************
 *  mng/config.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2007, 2009 Johannes Singler <singler@ira.uka.de>
 *  Copyright (C) 2008, 2009 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <fstream>
#include <stxxl/bits/mng/mng.h>
#include <stxxl/version.h>
#include <stxxl/bits/common/log.h>
#include <stxxl/bits/common/error_handling.h>

#ifdef BOOST_MSVC
 #include <windows.h>
#endif

__STXXL_BEGIN_NAMESPACE

static inline bool exist_file(const std::string& path)
{
    //STXXL_MSG("Checking " << path << " for disk configuration.");
    std::ifstream in(path.c_str());
    return in.good();
}

//! Parse a string like "343KB" or "44g" into the corresponding size in bytes
static inline int64 parse_disksize(const std::string& str, bool& ok)
{
    ok = false;

    char* endptr;
    int64 size = strtoul(str.c_str(), &endptr, 10);
    if (!endptr) return 0; // parse failed, no number

    while (endptr[0] == ' ') ++endptr; // skip over spaces

    if ( endptr[0] == 0 ) // number parsed, no suffix
        size *= 1000 * 1000;
    else if ( (endptr[0] == 'b' || endptr[0] == 'B') && endptr[1] == 0 ) // bytes
        size *= 1;
    // suffix powers of 10
    else if ( (endptr[0] == 'k' || endptr[0] == 'K') &&
              (endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) ) )
        size *= 1000;
    else if ( (endptr[0] == 'm' || endptr[0] == 'M') &&
              (endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) ) )
        size *= 1000 * 1000;
    else if ( (endptr[0] == 'g' || endptr[0] == 'G') &&
              (endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) ) )
        size *= 1000 * 1000 * 1000;
    else if ( (endptr[0] == 't' || endptr[0] == 'T') &&
              (endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) ) )
        size *= int64(1000) * int64(1000) * int64(1000) * int64(1000);
    else if ( (endptr[0] == 'p' || endptr[0] == 'P') &&
              (endptr[1] == 0 || ( (endptr[1] == 'b' || endptr[1] == 'B') && endptr[2] == 0) ) )
        size *= int64(1000) * int64(1000) * int64(1000) * int64(1000) * int64(1000);
    // suffix powers of 2
    else if ( (endptr[0] == 'k' || endptr[0] == 'K') &&
              (endptr[1] == 'i' || endptr[0] == 'I') &&
              (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
        size *= 1024;
    else if ( (endptr[0] == 'm' || endptr[0] == 'M') &&
              (endptr[1] == 'i' || endptr[0] == 'I') &&
              (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
        size *= 1024 * 1024;
    else if ( (endptr[0] == 'g' || endptr[0] == 'G') &&
              (endptr[1] == 'i' || endptr[0] == 'I') &&
              (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
        size *= 1024 * 1024 * 1024;
    else if ( (endptr[0] == 't' || endptr[0] == 'T') &&
              (endptr[1] == 'i' || endptr[0] == 'I') &&
              (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
        size *= int64(1024) * int64(1024) * int64(1024) * int64(1024);
    else if ( (endptr[0] == 'p' || endptr[0] == 'P') &&
              (endptr[1] == 'i' || endptr[0] == 'I') &&
              (endptr[2] == 0 || ( (endptr[2] == 'b' || endptr[2] == 'B') && endptr[3] == 0) ) )
        size *= int64(1024) * int64(1024) * int64(1024) * int64(1024) * int64(1024);
    else
        return 0;

    ok = true;
    return size;
}

config::config()
{
    // check different locations for disk configuration files

    // check STXXLCFG environment path
    const char* stxxlcfg = getenv("STXXLCFG");
    if (stxxlcfg && exist_file(stxxlcfg)) {
        init(stxxlcfg);
        return;
    }

    // read environment
    const char* hostname = getenv("HOSTNAME");
    const char* home = getenv("HOME");

    // check current directory
    {
        std::string basepath = "./.stxxl";

        if (hostname && exist_file(basepath + "." + hostname)) {
            init(basepath + "." + hostname);
            return;
        }

        if (exist_file(basepath)) {
            init("./.stxxl");
            return;
        }
    }

    // check home directory
    if (home)
    {
        std::string basepath = std::string(home) + "/.stxxl";

        if (hostname && exist_file(basepath + "." + hostname)) {
            init(basepath + "." + hostname);
            return;
        }

        if (exist_file(basepath)) {
            init("./.stxxl");
            return;
        }
    }

    // load default configuration
    init("");
}

void config::init(const std::string& config_path)
{
    logger::get_instance();
    STXXL_MSG(get_version_string());
    std::vector<DiskEntry> flash_props;
    std::ifstream cfg_file(config_path.c_str());
    if (!cfg_file)
    {
        STXXL_ERRMSG("Warning: no config file found.");
        STXXL_ERRMSG("Using default disk configuration.");
#ifndef BOOST_MSVC
        DiskEntry entry1 = { "/var/tmp/stxxl", "syscall", 1000 * 1024 * 1024, true, false };
#else
        DiskEntry entry1 = { "", "wincall", 1000 * 1024 * 1024, true, false };
        char * tmpstr = new char[255];
        stxxl_check_ne_0(GetTempPath(255, tmpstr), resource_error);
        entry1.path = tmpstr;
        entry1.path += "stxxl";
        delete[] tmpstr;
#endif
#if 0
        DiskEntry entry2 =
        { "/tmp/stxxl1", "mmap", 100 * 1024 * 1024, true };
        DiskEntry entry3 =
        { "/tmp/stxxl2", "simdisk", 1000 * 1024 * 1024, false };
#endif
        disks_props.push_back(entry1);
        //disks_props.push_back(entry2);
        //disks_props.push_back(entry3);
    }
    else
    {
        std::string line;

        while ( std::getline(cfg_file,line) )
        {
            if (line.size() == 0 || line[0] == '#') continue;

            std::vector<std::string> tmp = split(line, "=", 2);
            bool is_disk, ok;

            if ((is_disk = (tmp[0] == "disk")) || tmp[0] == "flash")
            {
                tmp = split(tmp[1], ",", 3);
                DiskEntry entry = {
                    tmp[0], tmp[2],
                    parse_disksize(tmp[1], ok),
                    false,
                    false
                };
                if (!ok) {
                    STXXL_THROW(std::runtime_error, "config::config",
                                "Invalid disk size '" << tmp[1] << "' in disk configuration file.");
                }
                if (entry.size == 0)
                    entry.autogrow = true;
                if (is_disk)
                    disks_props.push_back(entry);
                else
                    flash_props.push_back(entry);
            }
            else
            {
                STXXL_ERRMSG("Unknown configuration token " << tmp[0]);
            }
        }
        cfg_file.close();
    }

    // put flash devices after regular disks
    first_flash = disks_props.size();
    disks_props.insert(disks_props.end(), flash_props.begin(), flash_props.end());

    if (disks_props.empty())
        STXXL_THROW(std::runtime_error, "config::config", "No disks found in '" << config_path << "' .");

    int64 total_size = 0;
    for (std::vector<DiskEntry>::const_iterator it = disks_props.begin(); it != disks_props.end(); it++)
    {
        STXXL_MSG("Disk '" << (*it).path << "' is allocated, space: " <<
                  ((*it).size) / (1024 * 1024) <<
                  " MiB, I/O implementation: " << (*it).io_impl);

        total_size += (*it).size;
    }

    if (disks_props.size() > 1)
    {
        STXXL_MSG("In total " << disks_props.size() << " disks are allocated, space: " <<
                  (total_size / (1024 * 1024)) <<
                  " MiB");
    }
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
