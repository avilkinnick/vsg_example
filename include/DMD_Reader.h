#ifndef DMD_READER_H
#define DMD_READER_H

#include <vsg/all.h>

#include <string>

class DMD_Reader : public vsg::txt
{
public:
    DMD_Reader();

    vsg::ref_ptr<vsg::Object> read(
        const vsg::Path&,
        vsg::ref_ptr<const vsg::Options> = {}
    ) const override;

private:
    void remove_CR_symbols(std::string& str) const;
};

#endif // DMD_READER_H
