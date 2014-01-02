void TnsEncode(TnsInfo* tnsInfo,       /* TNS info */
               int numberOfBands,       /* Number of bands per window */
               int maxSfb,              /* max_sfb */
               enum WINDOW_TYPE blockType,   /* block type */
               int* sfbOffsetTable,     /* Scalefactor band offset table */
               float* spec); 