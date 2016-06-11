; This is where we define our sideload table

                AREA    HEADER, DATA, READONLY
                IMPORT  verify_cert

__Vectors       DCD     verify_cert
                
                END
