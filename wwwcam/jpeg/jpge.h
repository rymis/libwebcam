// jpge.h - C++ class for JPEG compression.
// Public domain, Rich Geldreich <richgel99@gmail.com>
// Alex Evans: Added RGBA support, linear memory allocator.
#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif				/* } */

typedef unsigned char uint8;
typedef signed short int16;
typedef signed int int32;
typedef unsigned short uint16;
typedef unsigned int uint32;

// JPEG chroma subsampling factors. Y_ONLY (grayscale images) and H2V2 (color images) are the most common.
enum subsampling { Y_ONLY = 0, H1V1 = 1, H2V1 = 2, H2V2 = 3 };

// JPEG compression parameters structure.
struct jpeg_params;
struct jpeg_params *jpeg_params_new(void);
void jpeg_params_free(struct jpeg_params *self);
int jpeg_params_check(const struct jpeg_params *params);
struct jpeg_params {
	// Quality: 1-100, higher is better. Typical values are around 50-95.
	int m_quality;

	// m_subsampling:
	// 0 = Y (grayscale) only
	// 1 = YCbCr, no subsampling (H1V1, YCbCr 1x1x1, 3 blocks per MCU)
	// 2 = YCbCr, H2V1 subsampling (YCbCr 2x1x1, 4 blocks per MCU)
	// 3 = YCbCr, H2V2 subsampling (YCbCr 4x1x1, 6 blocks per MCU-- very common)
	enum subsampling m_subsampling;

	// Disables CbCr discrimination - only intended for testing.
	// If true, the Y quantization table is also used for the CbCr channels.
	int m_no_chroma_discrim_flag;
	int m_two_pass_flag;
};

// Writes JPEG image to a file. 
// num_channels must be 1 (Y) or 3 (RGB), image pitch must be width*num_channels.
int compress_image_to_jpeg_file(const char *pFilename, int width,
				int height, int num_channels,
				const uint8 * pImage_data,
				const struct jpeg_params *comp_params);

// Writes JPEG image to memory buffer. 
// On entry, buf_size is the size of the output buffer pointed at by pBuf, which should be at least ~1024 bytes. 
// If return value is true, buf_size will be set to the size of the compressed data.
int compress_image_to_jpeg_file_in_memory(void *pBuf, int *buf_size,
					  int width, int height,
					  int num_channels,
					  const uint8 * pImage_data,
					  const struct jpeg_params
					  *comp_params);

// Output stream abstract class - used by the jpeg_encoder class to write to the output stream. 
// put_buf() is generally called with len==JPGE_OUT_BUF_SIZE bytes, but for headers it'll be called with smaller amounts.
struct jpeg_output_stream {
	void (*close) (void);
	int (*put_buf) (const void *pbuf, int len);
};

// Lower level jpeg_encoder class - useful if more control is needed than the above helper functions.
struct jpeg_encoder;
struct jpeg_encoder *jpeg_encoder_new();
void jpeg_encoder_free(struct jpeg_encoder *enc);

// Initializes the compressor.
// pStream: The stream object to use for writing compressed data.
// params - Compression parameters structure, defined above.
// width, height  - Image dimensions.
// channels - May be 1, or 3. 1 indicates grayscale, 3 indicates RGB source data.
// Returns false on out of memory or if a stream write fails.
int jpeg_encoder_encoder_init(struct jpeg_encoder *self,
			      struct jpeg_output_stream *pStream,
			      int width, int height, int src_channels,
			      const struct jpeg_params *comp_params);
void jpeg_encoder_deinit(struct jpeg_encoder *self);

// Call this method with each source scanline.
// width * src_channels bytes per scanline is expected (RGB or Y format).
// You must call with NULL after all scanlines are processed to finish compression.
// Returns false on out of memory or if a stream write fails.
int jpeg_encoder_process_scanline(struct jpeg_encoder *self,
				  const void *pScanline);
typedef int32 jpeg_sample_array_t;
struct jpeg_encoder {
	struct jpeg_output_stream *m_pStream;
	struct jpeg_params m_params;
	uint8 m_num_components;
	uint8 m_comp_h_samp[3], m_comp_v_samp[3];
	int m_image_x, m_image_y, m_image_bpp, m_image_bpl;
	int m_image_x_mcu, m_image_y_mcu;
	int m_image_bpl_xlt, m_image_bpl_mcu;
	int m_mcus_per_row;
	int m_mcu_x, m_mcu_y;
	uint8 *m_mcu_lines[16];
	uint8 m_mcu_y_ofs;
	jpeg_sample_array_t m_sample_array[64];
	int16 m_coefficient_array[64];
	int32 m_quantization_tables[2][64];
	unsigned m_huff_codes[4][256];
	uint8 m_huff_code_sizes[4][256];
	uint8 m_huff_bits[4][17];
	uint8 m_huff_val[4][256];
	uint32 m_huff_count[4][256];
	int m_last_dc_val[3];

#define JPGE_OUT_BUF_SIZE 2048
	uint8 m_out_buf[JPGE_OUT_BUF_SIZE];
	uint8 *m_pOut_buf;
	unsigned m_out_buf_left;
	uint32 m_bit_buffer;
	unsigned m_bits_in;
	uint8 m_pass_num;
	int m_all_stream_writes_succeeded;
};
void jpeg_encoder_optimize_huffman_table(struct jpeg_encoder *self,
					 int table_num, int table_len);
void jpeg_encoder_emit_byte(struct jpeg_encoder *self, uint8 i);
void jpeg_encoder_emit_word(struct jpeg_encoder *self, unsigned i);
void jpeg_encoder_emit_marker(struct jpeg_encoder *self, int marker);
void jpeg_encoder_emit_jfif_app0(struct jpeg_encoder *self);
void jpeg_encoder_emit_dqt(struct jpeg_encoder *self);
void jpeg_encoder_emit_sof(struct jpeg_encoder *self);
void jpeg_encoder_emit_dht(struct jpeg_encoder *self, uint8 * bits,
			   uint8 * val, int index, int ac_flag);
void jpeg_encoder_emit_dhts(struct jpeg_encoder *self);
void jpeg_encoder_emit_sos(struct jpeg_encoder *self);
void jpeg_encoder_emit_markers(struct jpeg_encoder *self);
void jpeg_encoder_compute_huffman_table(struct jpeg_encoder *self,
					unsigned *codes,
					uint8 * code_sizes,
					uint8 * bits, uint8 * val);
void jpeg_encoder_compute_quant_table(struct jpeg_encoder *self,
				      int32 * dst, int16 * src);
void jpeg_encoder_adjust_quant_table(struct jpeg_encoder *self,
				     int32 * dst, int32 * src);
void jpeg_encoder_first_pass_init(struct jpeg_encoder *self);
int jpeg_encoder_second_pass_init(struct jpeg_encoder *self);
int jpeg_encoder_jpg_open(struct jpeg_encoder *self, int p_x_res,
			  int p_y_res, int src_channels);
void jpeg_encoder_load_block_8_8_grey(struct jpeg_encoder *self, int x);
void jpeg_encoder_load_block_8_8(struct jpeg_encoder *self, int x,
				 int y, int c);
void jpeg_encoder_load_block_16_8(struct jpeg_encoder *self, int x, int c);
void jpeg_encoder_load_block_16_8_8(struct jpeg_encoder *self, int x, int c);
void jpeg_encoder_load_quantized_coefficients(struct jpeg_encoder *self,
					      int component_num);
void jpeg_encoder_flush_output_buffer(struct jpeg_encoder *self);
void jpeg_encoder_put_bits(struct jpeg_encoder *self, unsigned bits,
			   unsigned len);
void jpeg_encoder_code_coefficients_pass_one(struct jpeg_encoder *self,
					     int component_num);
void jpeg_encoder_code_coefficients_pass_two(struct jpeg_encoder *self,
					     int component_num);
void jpeg_encoder_code_block(struct jpeg_encoder *self, int component_num);
void jpeg_encoder_process_mcu_row(struct jpeg_encoder *self);
int jpeg_encoder_terminate_pass_one(struct jpeg_encoder *self);
int jpeg_encoder_terminate_pass_two(struct jpeg_encoder *self);
int jpeg_encoder_process_end_of_image(struct jpeg_encoder *self);
void jpeg_encoder_load_mcu(struct jpeg_encoder *self, const void *src);
void jpeg_encoder_clear(struct jpeg_encoder *self);
void jpeg_encoder_init(struct jpeg_encoder *self);
static inline const struct jpeg_params *jpeg_encoder_get_params(struct
								jpeg_encoder
								*self)
{
	return &self->m_params;
}

	// Deinitializes the compressor, freeing any allocated memory. May be called at any time.
static inline unsigned jpeg_encoder_get_total_passes(struct
						     jpeg_encoder
						     *self)
{
	return self->m_params.m_two_pass_flag ? 2 : 1;
}

static inline unsigned jpeg_encoder_get_cur_pass(struct jpeg_encoder
						 *self)
{
	return self->m_pass_num;
}

/* extern "C" { */
#ifdef __cplusplus
}
#endif

#endif
