#ifndef ANI_IO_H
#define ANI_IO_H

#include <string>

namespace ani
{
inline void remove_CR_symbols(std::string& str)
{
    while (true)
    {
        auto CR_pos = str.find('\r');
        if (CR_pos == std::string::npos)
        {
            break;
        }
        str.erase(CR_pos);
    }
}
}

#endif // ANI_IO_H
