#ifndef __HASHPIPE_ATA_DATA_FORMATS_RECORDER__
#define __HASHPIPE_ATA_DATA_FORMATS_RECORDER__

//this is header for testing (semi-hashpipe) version for voltage receiver.
//It shall be connected to the

#include <stdint.h>
#include  "hpguppi_ibverbs_pkt_thread.h"

#define VOLT_OUTER_HASHPIPE_BLOCKS_AT_A_TIME 2

#define RX_DATA_LEN (8192)
#define RX_BUFF_LEN (8)
#define RX_MSG_LEN (RX_DATA_LEN+RX_BUFF_LEN)

#define MAXANTS 4

#define SAMPLES_PER_PACKET (16)
#define POLS_PER_PACKET (2)
#define CHANS_PER_PACKET (256)
#define BYTES_PER_COMPLEX_SAMPLE (1)

#define ANT_NO_MASK (0x003f)
#define CHAN_NO_MASK (0x0fff)
#define PCKT_NO_MASK (0x03fffffffff)
#define VERSION_NO_MASK (0x00ff)
#define VOLT_VERSION_MASK (0x0080)

#define GET_ANT_NO(x) ((x) & ANT_NO_MASK)
#define GET_CHAN_NO(x) ((x >> 6) & CHAN_NO_MASK)
#define GET_PCKT_NO(x) ((x >> 18) & PCKT_NO_MASK)
#define GET_VERSION_NO(x) ((x >> 56) & VERSION_NO_MASK)
#define IS_VERSION_VOLT(x) ((x >> 56) & VOLT_VERSION_MASK)


#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

#define ATA_ANT_NOT_INITED 0xFFFF

// ATA header byte offset within (unpadded) packet
//TODO: not sure abbout that, it may need to be 0
#define PKT_OFFSET_ATA_VOLT_HEADER \
  (sizeof(struct ethhdr) + \
   sizeof(struct iphdr ) + \
   sizeof(struct udphdr))

// ATA payload byte offset within (unpadded) packet
#define PKT_OFFSET_ATA_VOLT_PAYLOAD \
  (PKT_OFFSET_ATA_VOLT_HEADER + 1*sizeof(uint64_t))


/*packet data (assuming that the data are properly aligned.
 * by calling -o IBVPKTSZ=8,8192. The first 8 bytes is the volt
 * packet header, followed by padding bytes. Finally, the actual packet payload
 */
struct ata_ibv_volt_pkt
{
    uint64_t header;
    uint8_t padding[PKT_ALIGNMENT_SIZE-sizeof(uint64_t)];
    uint8_t data[];
};

struct ata_obs_info {
  uint32_t nchan;
  // Total number of antennas in current subarray
  uint32_t nants;
  // Starting channel number to be processed
  int32_t schan;
  // valid antenna Array
  uint32_t antids[MAXANTS];
};

int ata_obs_info_init(struct ata_obs_info * info);
int get_ata_ant_index(const struct ata_obs_info info, const int32_t antid);

// Returns the largest power of two that it less than or equal to x.
// Returns 0 if x is 0.
static inline
uint32_t
prevpow2(uint32_t x)
{
  return x == 0 ? 0 : (0x80000000 >> __builtin_clz(x));
}

//number of pakets ids per block. We are not filling the entire block of data
static inline
uint32_t
ata_pktidx_per_block(const size_t block_size, const struct ata_obs_info info)
{
    uint32_t pblk = block_size/(info.nants*info.nchan*SAMPLES_PER_PACKET*POLS_PER_PACKET*CHANS_PER_PACKET*BYTES_PER_COMPLEX_SAMPLE);
    return prevpow2(pblk);
}

static inline
uint32_t
ata_block_size(const size_t block_size, const struct mk_obs_info info)
{
  return ata_pktidx_per_block(block_size, info) * (info.nants*info.nchan*SAMPLES_PER_PACKET*POLS_PER_PACKET*CHANS_PER_PACKET*BYTES_PER_COMPLEX_SAMPLE);
}

#endif
