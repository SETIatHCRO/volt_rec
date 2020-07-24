#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "data_formats.h"
#include "common.h"

#define MY_LINE_SIZE 100

#if RX_DATA_LEN != (SAMPLES_PER_PACKET * POLS_PER_PACKET * CHANS_PER_PACKET)
#error bad data size!
#endif

//SAMPLES_PER_PACKET POLS_PER_PACKET CHANS_PER_PACKET
int convert(char * datain,char * dataout[MAX_NO_ANTS], uint32_t nAnts, uint32_t nChanpkts)
{
	int iK, iL, iM;
	for (iK = 0; iK<nAnts;++iK)
	{
		for (iL = 0; iL < nChanpkts; ++iL)
		{
			for(iM = 0; iM < SAMPLES_PER_PACKET; ++iM)
			{
				memcpy(
				        dataout[iK] + iL*POLS_PER_PACKET * CHANS_PER_PACKET + iM*nChanpkts*POLS_PER_PACKET * CHANS_PER_PACKET,
				        datain + iK*(RX_DATA_LEN*nChanpkts) + iL*RX_DATA_LEN + iM * POLS_PER_PACKET * CHANS_PER_PACKET,
					sizeof(char) * CHANS_PER_PACKET * POLS_PER_PACKET
				      );
			}
		}
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char * fname;
	char fnamePart[PATH_MAX];
	char fnameOutTmp[PATH_MAX];
	char fnameOutTextTmp[PATH_MAX];
	size_t namelen;
	int out_fds[MAX_NO_ANTS], in_fd;
	char * datain;
	char * dataout[MAX_NO_ANTS];
	uint32_t nAnts;
	uint32_t antIDs[MAX_NO_ANTS];
	uint32_t nChanpkts;
	uint64_t pktStart;
	uint32_t chanStart;
	int iK;
	ssize_t inBufferLen, outBufferLen;
	int txt_fd;
	char txtline[MY_LINE_SIZE];
	int is_ok =1;

	if (argc != 2)
	{
		printf("usage: %s filename\n",argv[0]);
		return 1;
	}
	fname = argv[1];
	namelen = strlen(fname);
	if(namelen + 20 > PATH_MAX)
	{
		fprintf(stderr,"name %s too long (%ld)\n",fname,namelen);
		return 1;
	}
	if(namelen < 4 || strncmp(fname + namelen - 4,".bin",5))
	{
		fprintf(stderr,"it's not a \".bin\" file!");
		return 1;
	}
	strncpy(fnamePart,fname,namelen-4);
	fnamePart[namelen-4] = '\0';

	if((in_fd = open(fname,O_RDONLY)) < 0) error("open");

	//reading header
	if(read(in_fd,&nAnts,sizeof(uint32_t))!=sizeof(uint32_t)) error("read header");
	if(read(in_fd,antIDs,sizeof(uint32_t)*nAnts)!=(nAnts* sizeof(uint32_t))) error("read header");
	if(read(in_fd,&nChanpkts,sizeof(uint32_t))!=sizeof(uint32_t)) error("read header");
	if(read(in_fd,&chanStart,sizeof(uint32_t))!=sizeof(uint32_t)) error("read header");
	if(read(in_fd,&pktStart,sizeof(uint64_t))!=sizeof(uint64_t)) error("read header");
	
	if(nAnts >= MAX_NO_ANTS)
	{
		fprintf(stderr,"number of antennas (%d) higher than max (%d)\n",nAnts,MAX_NO_ANTS);
		close(in_fd);
		return 1;
	}

	//calculating buffer size
	inBufferLen = RX_DATA_LEN*sizeof(char)*nAnts*nChanpkts;
	outBufferLen = RX_DATA_LEN*sizeof(char)*nChanpkts;

	//alocating input buffer
	if(( datain = (char *) malloc (inBufferLen)) == NULL) error("malloc");
	for (iK = 0; iK < nAnts; ++iK)
	{
		//for each antenna, we are alocating data, generating file name, opening data file, opening text file and dumping header
		if ((dataout[iK] = (char *) malloc(outBufferLen)) == NULL) error("malloc");

		snprintf(fnameOutTextTmp,PATH_MAX,"%s_ant%u.txt",fnamePart,antIDs[iK]);
		if((txt_fd = open(fnameOutTextTmp,O_CREAT|O_WRONLY|O_TRUNC|S_IWUSR|S_IRUSR,0644)) < 0) error("open");

		snprintf(txtline,MY_LINE_SIZE,"antenna: %u\n",antIDs[iK]);
		if (write(txt_fd,txtline,strlen(txtline)) <= 0) error("text write");
		snprintf(txtline,MY_LINE_SIZE,"chan_start: %u\n",chanStart);
		if (write(txt_fd,txtline,strlen(txtline)) <= 0) error("text write");
		snprintf(txtline,MY_LINE_SIZE,"no_chan: %u\n",CHANS_PER_PACKET*nChanpkts);
		if (write(txt_fd,txtline,strlen(txtline)) <= 0) error("text write");
		snprintf(txtline,MY_LINE_SIZE,"pkt_start: %lu\n",pktStart);
		if (write(txt_fd,txtline,strlen(txtline)) <= 0) error("text write");

		close(txt_fd);

		snprintf(fnameOutTmp,PATH_MAX,"%s_ant%u.bin",fnamePart,antIDs[iK]);
		if((out_fds[iK] = open(fnameOutTmp,O_CREAT|O_WRONLY|O_TRUNC|S_IWUSR|S_IRUSR,0644)) < 0) error("open");

	}	

	while(is_ok)
	{
		if(read(in_fd,datain,inBufferLen) < inBufferLen)
		{
			is_ok = 0;
			break;
		}
		//huge assumption that inBufferLen, outBufferlen, SAMPLES_PER_PACKET POLS_PER_PACKET CHANS_PER_PACKET are correct
		convert(datain,dataout,nAnts,nChanpkts);
		for (iK = 0; iK < nAnts; ++iK)
		{
			if(write(out_fds[iK],dataout[iK],outBufferLen)<outBufferLen) error("write");
		}
	}

	for (iK = 0; iK < nAnts; ++iK)
	{
		close(out_fds[iK]);
	}
	close(in_fd);

	return 0;

}

