

#ifndef ULONGEST
#define ULONGEST unsigned long long
#endif

struct Dwarf_Universal_Head_s;
typedef struct Dwarf_Universal_Head_s *  Dwarf_Universal_Head;

int dwarf_object_detector_universal_head(
    char         *dw_path,
    ULONGEST      dw_filesize,
    ULONGEST     *dw_contentcount,
    Dwarf_Universal_Head * dw_head,
    int            *errcode);

int dwarf_object_detector_universal_instance(
    Dwarf_Universal_Head dw_head,
    ULONGEST  dw_index_of,
    ULONGEST *dw_cpu_type,
    ULONGEST *dw_cpu_subtype,
    ULONGEST *dw_offset,
    ULONGEST *dw_size,
    ULONGEST *dw_align,
    int         *errcode);
void dwarf_dealloc_universal_head(Dwarf_Universal_Head dw_head);
