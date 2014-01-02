 /*
============================================================================
 *  Copyright (C) 2012 Movidius Ltd.
 *
 *  All Rights Reserved
 *
============================================================================
 *
 * This library contains proprietary intellectual property of Movidius Ltd.
 * This source code is the property and confidential information of Movidius Ltd.
 * The library and its source code are protected by various copyrights
 * and portions may also be protected by patents or other legal protections.
 *
 * This software is licensed for use with the Myriad family of processors only.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE. IN NO EVENT SHALL THE COPYRIGHT OWNER BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * http://www.movidius.com/
 *
 */



#ifndef SPI_H_INCLUDED
#define SPI_H_INCLUDED

typedef struct {
    unsigned int CAPABILITY;
    unsigned int reserved[7];
    unsigned int MODE;
    unsigned int EVENT;
    unsigned int MASK;
    unsigned int CMD;
    unsigned int TDATA;
    unsigned int RDATA;
    unsigned int SLVSEL;
} SPI_t;

#define SPI_MODE_LOOPBACK	0x40000000
#define SPI_MODE_SPIMODE_3	0x30000000
#define SPI_MODE_SPIMODE_2	0x20000000
#define SPI_MODE_SPIMODE_1	0x10000000
#define SPI_MODE_SPIMODE_0	0x00000000
#define SPI_MODE_SPIMODE_MASK	0x30000000
#define SPI_MODE_SPIMODE_POS	28
#define SPI_MODE_SPIMODE_LEN	(29-28+1)
#define SPI_MODE_CLK_DIV16_EN	0x08000000
#define SPI_MODE_CLK_DIV16_DIS	0x00000000
#define SPI_MODE_DIR_MSB	0x04000000
#define SPI_MODE_REV		0x04000000
#define SPI_MODE_DIR_LSB	0x00000000
#define SPI_MODE_MASTER		0x02000000
#define SPI_MODE_SLAVE		0x00000000
#define SPI_MODE_EN		0x01000000
#define SPI_MODE_FSIZE_32BIT	0x00000000
#define SPI_MODE_FSIZE_16BIT	0x00F00000
#define SPI_MODE_FSIZE_8BIT	0x00700000
#define SPI_MODE_FSIZE_MASK	0x00F00000
#define SPI_MODE_FSIZE_POS	20
#define SPI_MODE_FSIZE_LEN	(23-20+1)
#define SPI_MODE_SCALER_MASK	0x000F0000
#define SPI_MODE_SCALER_POS	16
#define SPI_MODE_SCALER_LEN	(19-16+1)
#define SPI_MODE_3WIRE		0x00008000
#define SPI_MODE_CLOCK_GAP_NONE	0x00000000
#define SPI_MODE_CLOCK_GAP_MASK	0x00000F80
#define SPI_MODE_CLOCK_GAP_POS 7
#define SPI_MODE_CLOCK_GAP_LEN (11-7+1)

#define SPI_EVENT_LASTCHAR	0x4000
#define SPI_EVENT_OVERRUN	0x1000
#define SPI_EVENT_UNDERRUN	0x0800
#define SPI_EVENT_MULTIMASTER	0x0400
#define SPI_EVENT_RFNE		0x0200 // RX FIFO not empty
#define SPI_EVENT_TFNF		0x0100 // TX FIFO not full
#define SPI_EVENT_NONE		0x0000
#define SPI_CMD_LASTCHAR_EN	0x00400000

#define SPI_TDATA_8BIT_MSB(x)	((x)<<24)
#define SPI_TDATA_8BIT_LSB(x)	((x)&0xFF)

#define SPI_RDATA_8BIT_MSB(x)	(((x)>>16)&0xFF)
#define SPI_RDATA_8BIT_LSB(x)	(((x)>>8)&0xFF)

#define SPI_SLVSEL_NONE		0x0
#define SPI_SLVSEL_SS0		0x1
#define SPI_SLVSEL_SS1		0x2
#define SPI_SLVSEL_SS2		0x4
#define SPI_SLVSEL_SS3		0x8

#endif
