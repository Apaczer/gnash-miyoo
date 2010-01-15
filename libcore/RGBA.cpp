// types.h    -- Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#include "RGBA.h"
#include "GnashNumeric.h"
#include "log.h"
#include "SWFStream.h"
#include <sstream> 

namespace gnash {

void
rgba::read(SWFStream& in, SWF::TagType tag)
{
    switch (tag)
    {
        case SWF::DEFINESHAPE:
        case SWF::DEFINESHAPE2:
            read_rgb(in);
            break;
        default:
        case SWF::DEFINESHAPE3:
            read_rgba(in);
            break;
    }
}

void
rgba::read_rgba(SWFStream& in)
{
    read_rgb(in);
    in.ensureBytes(1);
    m_a = in.read_u8();
}

/// Can throw ParserException on premature end of input stream
void
rgba::read_rgb(SWFStream& in)
{
    in.ensureBytes(3);
    m_r = in.read_u8();
    m_g = in.read_u8();
    m_b = in.read_u8();
    m_a = 0x0FF;
}

std::string
rgba::toShortString() const
{
    std::stringstream ss;
    ss << (unsigned)m_r << ","
        << (unsigned)m_g << ","
        << (unsigned)m_b << ","
        << (unsigned)m_a;
    return ss.str();
}

void
rgba::set_lerp(const rgba& a, const rgba& b, float f)
{
    m_r = static_cast<boost::uint8_t>(frnd(flerp(a.m_r, b.m_r, f)));
    m_g = static_cast<boost::uint8_t>(frnd(flerp(a.m_g, b.m_g, f)));
    m_b = static_cast<boost::uint8_t>(frnd(flerp(a.m_b, b.m_b, f)));
    m_a = static_cast<boost::uint8_t>(frnd(flerp(a.m_a, b.m_a, f)));
}

rgba
colorFromHexString(const std::string& color)
{
    std::stringstream ss(color);
    boost::uint32_t hexnumber;
    
    if (!(ss >> std::hex >> hexnumber)) {
        log_error("Failed to convert string to RGBA value! This is a "
                "Gnash bug");
        return rgba();
    }

    rgba ret;
    ret.parseRGB(hexnumber);
    return ret;
}

std::ostream&
operator<<(std::ostream& os, const rgba& r)
{
    return os << "rgba: "
        << static_cast<unsigned>(r.m_r) << ","
        << static_cast<unsigned>(r.m_g) << ","
        << static_cast<unsigned>(r.m_b) << ","
        << static_cast<unsigned>(r.m_a);
}

} // namespace gnash


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
