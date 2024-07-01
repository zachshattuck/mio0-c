#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>

#define SIG_SIZE 4
// #define SIG    { 0x80, 0x37, 0x12, 0x40 } //0x80371240
// #define SIG_BS { 0x37, 0x80, 0x40, 0x12 } //0x37804012
const uint8_t SIG[SIG_SIZE] = { 0x80, 0x37, 0x12, 0x40 };
const uint8_t SIG_BS[SIG_SIZE] = { 0x37, 0x80, 0x40, 0x12 };

#define FILENAME argv[1]

const char outfile_prefix[] = "tex";

// Note to self:
// intel processors are little-endian
// ROM endiannesses:
//  - z64: big-endian
//  - v64: little-endian
//  - n64: little-endian and probably byte-swapped




void print_usage(FILE* stream, char* path) {
  fprintf(stream, "Usage: %s path/to/ROM", path);
}

int is_valid_sig(uint8_t* sig) {
  for(int i = 0; i < SIG_SIZE; i++) {
    if(sig[i] != SIG[i]) {
      return 0;
    }
  }

  return 1;
}

int is_valid_sig_bs(uint8_t* sig) {
  for(int i = 0; i < SIG_SIZE; i++) {
    if(sig[i] != ((uint8_t*)SIG_BS)[i]) {
      return 0;
    }
  }

  return 1;
}

void decompress_mio0_block(FILE* fp_rom, FILE* fp_out, int start) {
}

int main(int argc, char** argv) {

  if(argc != 2) {
    print_usage(stderr, argv[0]);
    return 1;
  }

  int result;
  int i;



  // Open file
  FILE* fp_rom = fopen(FILENAME, "rb");
  if(fp_rom == NULL) {
    fprintf(stderr, "Failed to open file: %s", FILENAME);
    return 1;
  }
  printf("Opened file: %s\n", FILENAME);



  // Get size of file
  struct stat st;
  result = stat(FILENAME, &st); // How can I use the FILE pointer from `fopen`?
  if(result != 0) {
    fprintf(stderr, "Failed to stat file: %s", FILENAME);
    return 1;
  }
  int sz = st.st_size;
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
      locations[count++] = i;
      continue;
    }

    // Return back to where we were for potential case of "MMMMMMIO0"
    fseek(fp_rom, 0L, i);
  }

  printf("Found %d instances of \"MIO0\"\n", count);





  for(i = 0; i < count; i++) {
    printf("DECOMPRESSING BLOCK 1: 0x%x ----------------\n", locations[i]);

    char filename[30];
    snprintf(filename, 30, "0x%x.texture", locations[i]);

    // Open output file for writing
    FILE* fp_outfile = fopen(filename, "wb");
    if(fp_outfile == NULL) {
      fprintf(stderr, "Failed to open file: %s", FILENAME);
      continue;
    }
    printf("Opened file: %s\n", FILENAME);

    decompress_mio0_block(fp_rom, fp_outfile, locations[i]);

    printf("\n");
    fclose(fp_outfile);
  }

  fclose(fp_rom);
}