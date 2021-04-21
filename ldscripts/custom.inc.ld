    . = extRamStart;
#ifdef CUSTOM_LOAD
    BEGIN_SEG(custom, .)
#else
    BEGIN_NOLOAD(custom_)
#endif
    {
        BUILD_DIR/src/custom/custom.o(.text);
        BUILD_DIR/src/custom/drawing.o(.text);
        BUILD_DIR/src/custom/custom.o(.*data*);
        BUILD_DIR/src/custom/drawing.o(.*data*);
    }
#ifdef CUSTOM_LOAD
    END_SEG(custom)
#else
    END_NOLOAD(custom_)
#endif
    BEGIN_NOLOAD(custom)
    {
        BUILD_DIR/src/custom/custom.o(.bss);
        BUILD_DIR/src/custom/drawing.o(.bss);
    }
    END_NOLOAD(custom)
