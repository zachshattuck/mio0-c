#include <stdio.h>
#include <stdint.h>

#define HEADER_SIZE 0x10
#define SIG_SIZE 4
const uint8_t SIG[SIG_SIZE] = { 0x80, 0x37, 0x12, 0x40 };
const uint8_t SIG_BS[SIG_SIZE] = { 0x37, 0x80, 0x40, 0x12 };

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


/**
 * Given a ROM file pointer, the size of the ROM file, and the location of the MIO0 block within the file,
 * this function will decompress the block.
 */
void mio0_decompress_f(FILE* fp_rom, unsigned int fp_rom_size, int start, uint8_t** out, uint8_t* out_size) {
  int i = start;
  fseek(fp_rom, start, SEEK_SET);

  if(i + HEADER_SIZE > fp_rom_size) {
    fprintf(stderr, "Invalid MIO0 block");
    return;
  }


  uint8_t header[HEADER_SIZE];
  for(i = 0; i < HEADER_SIZE; i++) {
    header[i] = fgetc(fp_rom);;
  }



  // Read some of the metadata from the header
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





  uint8_t* buffer = (uint8_t*)malloc(decompressed_length);

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
        buffer[bytes_written++] = fgetc(fp_rom);
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
          buffer[bytes_written + i] = buffer[bytes_written + i - off];
        }

        bytes_written += len;
      }


      if(bytes_written == decompressed_length) {
        break;
      }
      fseek(fp_rom, start+HEADER_SIZE+layout_idx, SEEK_SET);

      mask>>=1;
    }

  }


  *out = buffer;
  *out_size = decompressed_length;
}

// TODO:
// void mio0_compress(uint8_t** in, uint8_t** out) { }