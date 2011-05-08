/* fix-bcid.c
 *
 * 
 * Copyright (C) 2011 Kenny Millington
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/system_properties.h>

#include "mtdutils.h"

#define MISC_NAME "misc" 
#define BLOCK_SIZE	2048

int read_partition(const MtdPartition *partition, char *buf, 
                   size_t partition_size) {
    MtdReadContext *in;
    size_t total;
    int len;
    int attempts = 0;

    do {
        total = 0;
        in = mtd_read_partition(partition);

        while ((len = mtd_read_data(in, buf + total, BLOCK_SIZE)) > 0)
            total += len;

        mtd_read_close(in);

        if(total != partition_size)
            sleep(3);
    } while (total != partition_size && attempts++ < 5); 

    return total == partition_size;
}

int write_partition(const MtdPartition *partition, char *buf, 
                    size_t partition_size) {
    MtdWriteContext *out;
    size_t total = 0;
    int len;
    int attempts = 0;
    
    do {
        total = 0;
        out = mtd_write_partition(partition);

        while (total < partition_size && 
               (len = mtd_write_data(out, buf + total, BLOCK_SIZE)) > 0) {
            total += len;
        }

        mtd_write_close(out);

        if(total != partition_size) 
            sleep(3);
    
    } while(total != partition_size && attempts++ < 5);

    return total == partition_size;
}

int read_cid(char *buf, int len) {
    char pbuf[PROP_VALUE_MAX];
    
    if(!__system_property_get("ro.cid", pbuf))
        return 0;

    strncpy(buf, pbuf, len);
    return 1;
}

int main(int argc, char **argv)
{
    const MtdPartition *partition;
    char cid[9];
    char *misc;
    size_t partition_size;
    
    if(!read_cid(cid, sizeof(cid))) {
        fprintf(stderr, "ERROR: Could not get your CID!\n");
        return -1;
    }
    
    printf("Read your CID: %s\n", cid);

    if (mtd_scan_partitions() <= 0) {
        fprintf(stderr, "ERROR: Could not scan MTD partitions!\n");
        return -2;
    }
    
    partition = mtd_find_partition_by_name(MISC_NAME);
    
    if (partition == NULL) {
        fprintf(stderr, "ERROR: Could not find misc partition!\n");
        return -3;
    }
    
    if (mtd_partition_info(partition, &partition_size, NULL, NULL)) {
        fprintf(stderr, "ERROR: Could not read partition information!\n");
        return -4;
    }
    
    misc = calloc(partition_size, sizeof(char));
    
    if(!read_partition(partition, misc, partition_size)) {
        fprintf(stderr, "ERROR: Could not read misc partition!\n");
        return -5;
    }     

    strcpy(misc, cid);

    if(!write_partition(partition, misc, partition_size)) {
        fprintf(stderr, "ERROR: Could not write misc partition!\n");
        return -6;
    }
    
    if(!read_partition(partition, misc, partition_size)) {
        fprintf(stderr, "ERROR: Could not read misc for validation!\n");
        return -7;
    }

    if(strncmp(misc, cid, sizeof(cid))) {
        fprintf(stderr, "ERROR: Validation failed!\n");
        return -8;
    }

    free(misc);

    printf("Success: Backup CID has been patched into misc.\n");
    return 0;
}
