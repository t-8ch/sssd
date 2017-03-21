#ifndef __SSS_IOBUF_H_
#define __SSS_IOBUF_H_

#include <talloc.h>
#include <stdint.h>
#include <errno.h>

#include "util/util_errors.h"

struct sss_iobuf;

/*
 * @brief Allocate an empty IO buffer
 *
 * @param[in]  mem_ctx      The talloc context that owns the iobuf
 *
 * When this buffer is written into, but the capacity is exceeded, the write
 * function will return an error.
 *
 * @param[in]  mem_ctx      The talloc context that owns the iobuf
 * @param[in]  size         The size of the data buffer
 * @param[in]  capacity     The maximum capacity the buffer can grow into.
 *                          Use 0 for an 'unlimited' buffer that will grow
 *                          until SIZE_MAX/2.
 *
 * @return The newly created buffer on success or NULL on an error.
 *
 */
struct sss_iobuf *sss_iobuf_init_empty(TALLOC_CTX *mem_ctx,
                                       size_t size,
                                       size_t capacity);

/*
 * @brief Allocate an IO buffer with a fixed size
 *
 * This function is useful for parsing an input buffer from an existing
 * buffer pointed to by data.
 *
 * The iobuf does not assume ownership of the data buffer in talloc terms,
 * but copies the data instead.
 *
 * @param[in]  mem_ctx      The talloc context that owns the iobuf
 * @param[in]  data         The data to initialize the IO buffer with. This
 *                          data is copied into the iobuf-owned buffer.
 * @param[in]  size         The size of the data buffer
 *
 * @return The newly created buffer on success or NULL on an error.
 */
struct sss_iobuf *sss_iobuf_init_readonly(TALLOC_CTX *mem_ctx,
                                          uint8_t *data,
                                          size_t size);

/*
 * @brief Returns the number of bytes currently stored in the iobuf
 *
 * @return The number of bytes (the data pointer offset)
 */
size_t sss_iobuf_get_len(struct sss_iobuf *iobuf);

/*
 * @brief Returns the capacity of the IO buffer
 *
 * @return The capacity of the IO buffer. Returns zero
 * for an unlimited buffer.
 */
size_t sss_iobuf_get_capacity(struct sss_iobuf *iobuf);

/*
 * @brief Returns the current size of the IO buffer
 */
size_t sss_iobuf_get_size(struct sss_iobuf *iobuf);

/*
 * @brief Returns the data pointer of the IO buffer
 */
uint8_t *sss_iobuf_get_data(struct sss_iobuf *iobuf);

/*
 * @brief Read from an IO buffer
 *
 * Read up to len bytes from an IO buffer. It is not an error to request
 * more bytes than the buffer actually has - the function will succeed, but
 * return the actual number of bytes read. Reading from an empty buffer just
 * returns zero bytes read.
 *
 * @param[in]  iobuf        The IO buffer to read from
 * @param[in]  len          The maximum number of bytes to read
 * @param[out] _buf         The buffer to read data into from iobuf
 * @param[out] _read        The actual number of bytes read from IO buffer.
 *
 * @return EOK on success, errno otherwise
 */
errno_t sss_iobuf_read(struct sss_iobuf *iobuf,
                       size_t len,
                       uint8_t *_buf,
                       size_t *_read);

/*
 * @brief Write into an IO buffer
 *
 * Attempts to write len bytes into the iobuf. If the capacity is exceeded,
 * the iobuf module tries to extend the buffer up to the maximum capacity.
 *
 * If reallocating the internal buffer fails, the data pointers are not
 * touched.
 *
 * @param[in]  iobuf        The IO buffer to write to
 * @param[in]  buf          The data to write into the buffer
 * @param[in]  len          The number of bytes to write
 *
 * @return EOK on success, errno otherwise. Notably returns ENOBUFS if
 * the buffer capacity is exceeded.
 */
errno_t sss_iobuf_write_len(struct sss_iobuf *iobuf,
                            uint8_t *buf,
                            size_t len);

#endif /* __SSS_IOBUF_H_ */