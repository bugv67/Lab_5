#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

extern int startup(int argc, char **argv, void (*start)());

int translate_prot_flag(int elf_flags)
{
    // transelating elf flag ==> mmap flag
    int prot = 0;

    // loking on the 4 flag= reading
    if ((elf_flags & 4) != 0)
    {
        prot = prot + 1;
    }

    // reading
    if ((elf_flags & 2) != 0)
    {
        prot = prot + 2;
    }

    // excute
    if ((elf_flags & 1) != 0)
    {
        prot = prot + 4;
    }

    return prot;
}

void print_phdr_info(Elf32_Phdr *phdr, int arg)
{ /// print the line with info
    int prot = translate_prot_flag(phdr->p_flags);
    int map_flags = 18;

    printf("0x%04x  0x%06x 0x%08x 0x%08x 0x%05x   0x%05x    %d    0x%x    %d   %d \n",
           phdr->p_type,
           phdr->p_offset,
           phdr->p_vaddr,
           phdr->p_paddr,
           phdr->p_filesz,
           phdr->p_memsz,
           phdr->p_flags,
           phdr->p_align,
           prot, // translation of the flags
           map_flags);
}

// loading prigram to memory
void load_phdr(Elf32_Phdr *phdr, int fd)
{
    if (phdr->p_type == PT_LOAD)
    { // the only headrers we want to load

        // calc the right addresses- need to loading
        void *vaddr = (void *)(phdr->p_vaddr & 0xfffff000);
        off_t offset = phdr->p_offset & 0xfffff000;
        int padding = phdr->p_vaddr & 0xfff;

        // transelate
        int prot = translate_prot_flag(phdr->p_flags);

        printf("Mapped segment from file offset 0x%lx to virtual address %p (size: 0x%x)\n",
               (long)offset, vaddr, phdr->p_memsz + padding);
        // making the call, mapping,
        void *map = mmap(vaddr, phdr->p_memsz + padding, prot, MAP_PRIVATE | MAP_FIXED, fd, offset);

        if (map == MAP_FAILED)
        {
            perror("mmap failed");
            exit(1);
        }
        print_phdr_info(phdr, fd);
    }
}

/**
 * Iterates over all program headers in the ELF file.
 * map_start: The virtual memory address where the ELF file is mapped.
 * func: A callback function applied to each program header.
 * arg: Additional argument passed to the callback.
 */
int foreach (void *map_start, void (*func)(Elf32_Phdr *, int), int arg)
{
    // Cast the start to ELF Header to access meta-data
    Elf32_Ehdr *elf_head = (Elf32_Ehdr *)map_start;

    // Calculate the address of the Program Header Table (PHT)
    // We cast to (char*) to perform byte-level arithmetic
    Elf32_Phdr *phdr_table = (Elf32_Phdr *)((char *)map_start + elf_head->e_phoff);

    // Iterate through the table using the count provided in the ELF header
    for (int i = 0; i < elf_head->e_phnum; i++)
    {
        func(&phdr_table[i], arg);
    }

    return 0;
}
int main(int argc, char **argv)
{
    // Validate command line arguments
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <elf_file>\n", argv[0]);
        return 1;
    }

    // Open the ELF file in Read-Only mode
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Error: Could not open file");
        return 1;
    }

    // Determine file size using lseek (alternative to fstat)
    off_t file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET); // Reset file pointer to the beginning

    if (file_size < 0)
    {
        perror("Error: lseek failed");
        close(fd);
        return 1;
    }

    // Map the file into memory using mmap
    // MAP_PRIVATE ensures changes to the memory are not written back to the file
    void *map_start = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map_start == MAP_FAILED)
    {
        perror("Error: mmap failed");
        close(fd);
        return 1;
    }
    printf("Type     Offset   VirtAddr   PhysAddr   FileSiz   MemSiz   Flg   Align  Prot   Map \n");

    foreach (map_start, load_phdr, fd)
        ;

    // finding the entry point
    Elf32_Ehdr *elf_head = (Elf32_Ehdr *)map_start;

    startup(argc - 1, argv + 1, (void *)(elf_head->e_entry));
    return 0;
}