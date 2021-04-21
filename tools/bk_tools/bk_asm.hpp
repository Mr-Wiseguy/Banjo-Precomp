#include "n64_span.h"
#include "libdeflate/libdeflate.h"

class bk_asm{
    public:
        bk_asm(const n64_span& span);
        bk_asm(const n64_span& code_span, const n64_span& data_span);
        ~bk_asm(void);
        const n64_span& code(void) const;
        const n64_span& data(void) const;
        const n64_span& compressed(int lvl = 6);

    private:
        void _comp_method(int lvl) const;
        mutable n64_span *_comp_span = nullptr;
        mutable uint8_t *_comp_buff = nullptr;

        void _decomp_method(void) const;
        mutable libdeflate_decompressor* decomper = nullptr;
        mutable n64_span *_code_span = nullptr;
        mutable uint8_t *_code_buff = nullptr;

        mutable n64_span *_data_span = nullptr;
        mutable uint8_t *_data_buff = nullptr;
};
