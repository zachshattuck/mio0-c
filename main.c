#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>

#define HEADER_SIZE 0x10
#define SIG_SIZE 4
const uint8_t SIG[SIG_SIZE] = { 0x80, 0x37, 0x12, 0x40 };
const uint8_t SIG_BS[SIG_SIZE] = { 0x37, 0x80, 0x40, 0x12 };

#define ROM_FILENAME argv[1]

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


void decompress_mio0_block(FILE* fp_rom, unsigned int fp_rom_size, int start) {
  int i = start;
  fseek(fp_rom, start, SEEK_SET);

  if(i + 0x10 > fp_rom_size) {
    fprintf(stderr, "Invalid MIO0 block");
    return;
  }

  uint8_t header[0x10];
  for(i = 0; i < 0x10; i++) {
    header[i] = fgetc(fp_rom);;
  }

  uint32_t decompressed_length;
  uint32_t compressed_offset;
  uint32_t uncompressed_offset;

  decompressed_length = (
    (header[0x4] << 24)
    | (header[0x5] << 16)
    | (header[0x6] << 8)
    | (header[0x7])
  );

  compressed_offset = (
    (header[0x8] << 24)
    | (header[0x9] << 16)
    | (header[0xA] << 8)
    | (header[0xB])
  );

  uncompressed_offset = (
    (header[0xC] << 24)
    | (header[0xD] << 16)
    | (header[0xE] << 8)
    | (header[0xF])
  );

  printf("Decompressed length: %d bytes\n", decompressed_length);
  printf("Compressed offset: %d bytes\n", compressed_offset);
  printf("Decompressed offset: %d bytes\n", uncompressed_offset);

  if(start+compressed_offset > fp_rom_size) {
    fprintf(stderr, "Compressed offset is past EOF");
    return;
  }

  if(start+uncompressed_offset > fp_rom_size) {
    fprintf(stderr, "Uncompressed offset is past EOF");
    return;
  }
  uint8_t* output = (uint8_t*)malloc(decompressed_length);


  int bytes_written = 0;

  uint8_t layout_byte;
  uint8_t layout_bit;
  uint8_t mask;

  int layout_idx = 0;
  int uncompressed_idx = 0;
  int compressed_idx = 0;

  int error = 0;

  while(bytes_written < decompressed_length) {
    layout_byte = fgetc(fp_rom);
    layout_idx++;
    mask = 0b10000000;

    while(mask > 0) {
      layout_bit = layout_byte&mask;

      if(layout_bit) {

        fseek(fp_rom, start+uncompressed_offset+uncompressed_idx, SEEK_SET);
        output[bytes_written++] = fgetc(fp_rom);
        uncompressed_idx++;

      } else {

        uint8_t data[2];

        fseek(fp_rom, start+compressed_offset+compressed_idx, SEEK_SET);
        data[0] = fgetc(fp_rom);
        data[1] = fgetc(fp_rom);
        compressed_idx += 2;

        uint8_t  len = ((data[0] & 0xF0) >> 4) + 3;
        uint16_t off = (((data[0] & 0x0F) << 8) | data[1]) + 1;


        if(off > bytes_written) {
          fprintf(stderr, " !! Offset greater than bytes written.\n");
          printf("bw: %d, len: %d, off: %d\n", bytes_written, len, off);
          error = 1;
          bytes_written = decompressed_length;
          break;
        }

        if(bytes_written + len > decompressed_length)  {
          fprintf(stderr, " !! Writing %d bytes would be overflow\n", len);
          printf("bw: %d, len: %d, off: %d\n", bytes_written, len, off);
          error = 1;
          bytes_written = decompressed_length;
          break;
        }

        for(i = 0; i < len; i++) {
          output[bytes_written + i] = output[bytes_written + i - off];
        }

        bytes_written += len;
      }


      if(bytes_written == decompressed_length) {
        break;
      }
      fseek(fp_rom, start+0x10+layout_idx, SEEK_SET);

      mask>>=1;
    }

  }


  if(!error) {
    char filename[30];
    snprintf(filename, 30, "0x%x.texture", start);

    // Open output file for writing
    FILE* fp_outfile = fopen(filename, "wb");
    if(fp_outfile == NULL) {
      fprintf(stderr, "Failed to open file: %s", filename);
    } else {
      fwrite(output, decompressed_length, sizeof(uint8_t), fp_outfile);
      printf(" ----> Wrote to file %s!\n", filename);
    }

    fclose(fp_outfile);
  }


  free(output);
}

int main(int argc, char** argv) {

  if(argc != 2) {
    print_usage(stderr, argv[0]);
    return 1;
  }

  int result;
  int i;



  // Open file
  FILE* fp_rom = fopen(ROM_FILENAME, "rb");
  if(fp_rom == NULL) {
    fprintf(stderr, "Failed to open file: %s", ROM_FILENAME);
    return 1;
  }
  printf("Opened file: %s\n", ROM_FILENAME);



  // Get size of file
  struct stat st;
  result = stat(ROM_FILENAME, &st); // How can I use the FILE pointer from `fopen`?
  if(result != 0) {
    fprintf(stderr, "Failed to stat file: %s", ROM_FILENAME);
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


    decompress_mio0_block(fp_rom, sz, locations[i]);

    printf("\n");
  }

  fclose(fp_rom);
}