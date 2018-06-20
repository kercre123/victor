SECTIONS {
    . = 0x20005800;
    .data : {
        lib/boot.o (.data);
        * (.data);
    }
    .text : { 
        * (.text); 
    }
}
