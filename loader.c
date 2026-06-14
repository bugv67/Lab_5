#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

/**
 * Print information about each program header.
 * The static counter 'i' tracks the header index across function calls.
 */
void print_phdr(Elf32_Phdr *phdr, int arg)
{
    static int i = 0;
    printf("Program header number %d at address %p\n", i, (void *)phdr);
    i++;
}

/**
 * Iterates over all program headers in the ELF file.
 * map_start: The virtual memory address where the ELF file is mapped.
 * func: A callback function applied to each program header.
 * arg: Additional argument passed to the callback.
 */
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg)
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

    // Execute the iterator with the print callback
    foreach_phdr(map_start, print_phdr, fd);

    // Cleanup resources
    munmap(map_start, file_size);
    close(fd);

    return 0;
}