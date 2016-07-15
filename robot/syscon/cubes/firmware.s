                AREA    |.text|, DATA, READONLY
                ALIGN

                EXPORT  __CUBE_XS
                EXPORT  __CUBE_XS6

__CUBE_XS
                INCBIN  ../../accessories/releases/xs-p5.safe
__CUBE_XS6
                INCBIN  ../../accessories/releases/xs6-p3.safe

                END
