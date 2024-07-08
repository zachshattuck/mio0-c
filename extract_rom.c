#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "mio0.c"

const char outfile_prefix[] = "tex";



// Read at most `n` characters (newline included) into `str`.
// If present, the newline is removed (replaced by the null terminator).
void s_gets(char* str, int n)
{
  char* str_read = fgets(str, n, stdin);
  if (!str_read) return;

  int i = 0;
  while (str[i] != '\n' && str[i] != '\0') i++;

  if (str[i] == '\n') str[i] = '\0';
}

void print_usage(FILE* stream, char* path) {
  fprintf(stream, "Usage: %s path/to/ROM", path);
}


int main(int argc, char** argv) {

  char filename[255];

  if(argc > 2) {
    print_usage(stderr, argv[0]);
    return -1;
  }

  if(argc == 2) {
    int i = 0;
    while(argv[1][i] != '\0' && argv[1][i] != '\0') {
      filename[i] = argv[1][i];
      i++;
    }
    filename[i] = '\0';
  } else {
    printf("Enter ROM path: ");
    s_gets(filename, 255);
    printf("\n");
  }

  int result;
  int i;



  // Open file
  FILE* fp_rom = fopen(filename, "rb");
  if(fp_rom == NULL) {
    fprintf(stderr, "Failed to open file: %s", filename);
    return 1;
  }
  printf("Opened file: %s\n", filename);



  // Get size of file
  struct stat st;
  result = stat(filename, &st); // How can I use the FILE pointer from `fopen`?
  if(result != 0) {
    fprintf(stderr, "Failed to stat file: %s", filename);
    return 1;
  }
  unsigned int sz = st.st_size;
  printf("File size: %d bytes\n", sz);
  if(sz < SIG_SIZE) {
    fprintf(stderr, "Invalid ROM file: Too small.\n");
    return 1;
  }


  // Read signature
  fseek(fp_rom, 0L, SEEK_SET);
  uint8_t sig[SIG_SIZE];
  fread(sig, sizeof(uint8_t), 4, fp_rom);


  // Print signature
  printf("ROM signature: 0x");
  for(i = 0; i < SIG_SIZE; i++) {
    printf("%2x", sig[i]);
  }
  printf("\n");



  // Ensure signature is not byte-swapped (I would love to support this someday)
  int is_valid = is_valid_sig(sig);
  if(!is_valid) {
    int is_bs = is_valid_sig_bs(sig);
    if(is_bs) {
      fprintf(stderr, "This is a byte-swapped ROM. Please use '.z64' format.\n");
    } else {
      fprintf(stderr, "Invalid ROM file: Unknown signature.\n");
    }

    return 1;
  }
  printf("Valid signature.\n");




  // Count instances of MIO0
  int count = 0;
  int locations[100];
  uint8_t c;
  for(i = SIG_SIZE; i < sz; i++) {
    c = fgetc(fp_rom); // This could technically be EOF (-1)
    if(count == 100) {
      printf("Reached max MIO0 sections (because I'm lazy)\n");
      break;
    }

    if(c != 'M') continue;

    if(
      (fgetc(fp_rom) == 'I')
      && (fgetc(fp_rom) == 'O')
      && (fgetc(fp_rom) == '0')
    ) {
      locations[count++] = i-1;
      continue;
    }

    // Return back to where we were for potential case of "MMMMMMIO0"
    fseek(fp_rom, i, SEEK_SET);
  }

  printf("Found %d instances of \"MIO0\"\n\n", count);





  for(i = 0; i < count; i++) {
    printf("DECOMPRESSING BLOCK %d: 0x%x ----------------\n", i, locations[i]);


    uint8_t* output = NULL;
    uint8_t size;
    mio0_decompress_f(fp_rom, sz, locations[i], &output, &size);

    if(output == NULL) {
      printf("  ! `output` ptr was `NULL`\n\n");
      continue;
    }

    char filename[30];
    snprintf(filename, 30, "./out/0x%x.texture", locations[i]);

    // Open output file for writing
    FILE* fp_outfile = fopen(filename, "wb");

    if(fp_outfile == NULL) {
      fprintf(stderr, "Failed to open file: %s", filename);
    } else {
      fwrite(output, size, sizeof(uint8_t), fp_outfile);
      printf(" ----> Wrote to file %s!\n", filename);
    }

    free(output);
    fclose(fp_outfile);

    printf("\n");
  }

  fclose(fp_rom);
}
