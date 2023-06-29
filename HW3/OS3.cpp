#include "filesystem.hpp"

bool compare(ext2_inode inode1, ext2_inode inode2, uint32_t inode_no) {
	bool retval = true;
	if (inode1.mode != inode2.mode) {
		retval = false;
		cout << "inode mode " << inode_no << " " << inode1.mode << " " << inode2.mode << endl;
	}
	if (inode1.uid != inode2.uid) {
		retval = false;
		cout << "inode uid " << inode_no << " " << inode1.uid << " " << inode2.uid << endl;
	}
	if (inode1.size != inode2.size) {
		retval = false;
		cout << "inode size " << inode_no << " " << inode1.size << " " << inode2.size << endl;
	}
	if (inode1.gid != inode2.gid) {
		retval = false;
		cout << "inode gid " << inode_no << " " << inode1.gid << " " << inode2.gid << endl;
	}
	if (inode1.link_count != inode2.link_count) {
		retval = false;
		cout << "inode link_count " << inode_no << " " << inode1.link_count << " " << inode2.link_count << endl;
	}
	if (inode1.block_count_512 != inode2.block_count_512) {
		retval = false;
		cout << "inode block_count_512 " << inode_no << " " << inode1.block_count_512 << " " << inode2.block_count_512 << endl;
	}
	if (inode1.flags != inode2.flags) {
		retval = false;
		cout << "inode flags " << inode_no << " " << inode1.flags << " " << inode2.flags << endl;
	}
	if (inode1.reserved != inode2.reserved) {
		retval = false;
		cout << "inode reserves" << inode_no << " " << inode1.reserved << " " << inode2.reserved << endl;
	}
	for (uint32_t i = 0; i < EXT2_NUM_DIRECT_BLOCKS; i++) {
		if (inode1.direct_blocks[i] != inode2.direct_blocks[i]) {
			retval = false;
			cout << "inode direct_block_" << i << " " << inode_no << " " << inode1.direct_blocks[i] << " " << inode2.direct_blocks[i] << endl;
		}
	}
	if (inode1.single_indirect != inode2.single_indirect) {
		retval = false;
		cout << "inode single_indirect " << inode_no << " " << inode1.single_indirect << " " << inode2.single_indirect << endl;
	}
	if (inode1.double_indirect != inode2.double_indirect) {
		retval = false;
		cout << "inode double_indirect " << inode_no << " " << inode1.double_indirect << " " << inode2.double_indirect << endl;
	}
	if (inode1.triple_indirect != inode2.triple_indirect) {
		retval = false;
		cout << "inode triple_indirect " << inode_no << " " << inode1.triple_indirect << " " << inode2.triple_indirect << endl;
	}

	if ((inode1.deletion_time != 0) != (inode2.deletion_time != 0)) {
		retval = false;
		cout << "inode deletion_time " << inode_no << " " << inode1.deletion_time << " " << inode2.deletion_time << endl;
	}
	return retval;
}

bool compare(ext2_block_group_descriptor desc1, ext2_block_group_descriptor desc2, uint32_t desc_no) {
	bool retval = true;
	if (desc1.block_bitmap != desc2.block_bitmap) {
		retval = false;
		cout << "descriptor block_bitmap " << desc_no << " " << desc1.block_bitmap << " " << desc2.block_bitmap << endl;
	}
	if (desc1.inode_bitmap != desc2.inode_bitmap) {
		retval = false;
		cout << "descriptor inode_bitmap " << desc_no << " " << desc1.inode_bitmap << " " << desc2.inode_bitmap << endl;
	}
	if (desc1.inode_table != desc2.inode_table) {
		retval = false;
		cout << "descriptor inode_table " << desc_no << " " << desc1.inode_table << " " << desc2.inode_table << endl;
	}
	if (desc1.free_inode_count != desc2.free_inode_count) {
		retval = false;
		cout << "descriptor free_inode_count " << desc_no << " " << desc1.free_inode_count << " " << desc2.free_inode_count << endl;
	}
	if (desc1.free_block_count != desc2.free_block_count) {
		retval = false;
		cout << "descriptor free_block_count" << desc_no << " " << desc1.free_block_count << " " << desc2.free_block_count << endl;
	}
	if (desc1.used_dirs_count != desc2.used_dirs_count) {
		retval = false;
		cout << "descriptor used_dirs_count " << desc_no << " " << desc1.used_dirs_count << " " << desc2.used_dirs_count << endl;
	}
	return retval;
}

bool compare(ext2_super_block super1, ext2_super_block super2) {
	bool retval = true;
	if (super1.inode_count != super2.inode_count) {
		retval = false;
		cout << "super inode_count " << super1.inode_count << " " << super2.inode_count << endl;
	}
	if (super1.block_count != super2.block_count) {
		retval = false;
		cout << "super block_count " << super1.block_count << " " << super2.block_count << endl;
	}
	if (super1.reserved_block_count != super2.reserved_block_count) {
		retval = false;
		cout << "super reserved_block_count " << super1.reserved_block_count << " " << super2.reserved_block_count << endl;
	}
	if (super1.free_block_count != super2.free_block_count) {
		retval = false;
		cout << "super free_block_count " << super1.free_block_count << " " << super2.free_block_count << endl;
	}
	if (super1.free_inode_count != super2.free_inode_count) {
		retval = false;
		cout << "super free_inode_count " << super1.free_inode_count << " " << super2.free_inode_count << endl;
	}
	if (super1.inode_count != super2.inode_count) {
		retval = false;
		cout << "super inode_count " << super1.inode_count << " " << super2.inode_count << endl;
	}
	if (super1.inode_size != super2.inode_size) {
		retval = false;
		cout << "super inode_size " << super1.inode_size << " " << super2.inode_size << endl;
	}
	if (super1.blocks_per_group != super2.blocks_per_group) {
		retval = false;
		cout << "super blocks_per_group " << super1.blocks_per_group << " " << super2.blocks_per_group << endl;
	}
	if (super1.inodes_per_group != super2.inodes_per_group) {
		retval = false;
		cout << "super inodes_per_group " << super1.inodes_per_group << " " << super2.inodes_per_group << endl;
	}
	return retval;
}

bool compare(filesystem file1, filesystem file2) {
	bool retval = true;
	if (!compare(file1.super_block, file2.super_block)) {
		retval = false;
		cout << "super_block" << endl;
	}
	for (unsigned int i = 0; i < file1.number_of_block_groups; i++) {
		if (!compare(file1.block_groups[i], file2.block_groups[i], i)) {
			cout << "block descriptor " << i << endl;
			retval = false;
		}
		for (unsigned int j = 0; j < file1.block_size; j++) {
			if (file1.block_bitmaps[i][j] != file2.block_bitmaps[i][j]) {
				cout << "block_bitmap " << dec << i << " " << j << " " << hex << (int)file1.block_bitmaps[i][j] << " " << (int)file2.block_bitmaps[i][j] << endl;
				retval = false;
			}
				
			if (file1.inode_bitmaps[i][j] != file2.inode_bitmaps[i][j]) {
				cout << "inode_bitmap " << dec << i << " " << j << " " << hex << (int)file1.inode_bitmaps[i][j] << " " << (int)file2.inode_bitmaps[i][j] << endl;
				retval = false;
			}
		}
	}

	for (unsigned int i = 1; i < file1.super_block.inode_count; i++) {
		if (!compare(file1.getInode(i), file2.getInode(i), i)) {
			retval = false;
			cout << "inode " << dec << i << endl;
		}
	}
	return retval;
}

int main(int argc, char* argv[])
{
	if (string(argv[2]) == "mkdir") {
		filesystem f(argv[1]);
		f.ext2_mkdir(argv[3]);
		f.destroy_system();
	}
	else if (string(argv[2]) == "rmdir") {
		filesystem f(argv[1]);
		f.ext2_rmdir(argv[3]);
		f.destroy_system();
	}
	else if (string(argv[2]) == "rm") {
		filesystem f(argv[1]);
		f.ext2_rm(argv[3]);
		f.destroy_system();
	}
	else if (string(argv[2]) == "ed") {
		filesystem f(argv[1]);
		f.ext2_ed(argv[7], string(argv[9]), atoi(argv[4]), atoi(argv[6]));
		f.destroy_system();
	}
	else if (string(argv[2]) == "cmp") {
		filesystem f1(argv[1]);
		filesystem f2(argv[3]);
		cout << (int)compare(f1, f2) << endl;
		f1.destroy_system();
		f2.destroy_system();
	}
	else if (string(argv[2]) == "super") {
		filesystem f1(argv[1]);
		print_super_block(&f1.super_block);
		f1.destroy_system();
	}
	else if (string(argv[2]) == "inode") {
		filesystem f1(argv[1]);
		ext2_inode inode = f1.getInode(atoi(argv[3]));
		print_inode(&inode, atoi(argv[3]));
		f1.destroy_system();
	}
	else if (string(argv[2]) == "bitmap") {
		filesystem f1(argv[1]);
		f1.print_bitmaps((bitmap_type)atoi(argv[3]), atoi(argv[4]));
		f1.destroy_system();
	}
	
}
