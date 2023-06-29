#include "ext2fs_print.h"
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <iomanip>

using namespace std;

vector<string> tokenizePath(const char* path) {
	string s(path);
	vector<string> elements;
	string temp;

	for (int i = 0; i < int(s.size()); i++)
	{
		string test = s.substr(i, 1);

		if (test == "/")
		{
			if (!temp.empty())
			{
				if(temp != "C:" && temp != "msys64")
					elements.push_back(temp);
				temp.clear();
			}
		}
		else if (i + 1 >= s.size())
		{
			temp += s.substr(i, 1);
			elements.push_back(temp);
			break;
		}
		else
		{
			temp += s[i];
		}
	}
	return elements;
}

struct filesystem {
	unsigned int fd;
	ext2_super_block super_block;
	uint32_t number_of_block_groups;
	uint32_t block_size;
	ext2_inode root_inode;
	ext2_block_group_descriptor* block_groups;
	unsigned char** block_bitmaps;
	unsigned char** inode_bitmaps;

	uint32_t getBlockAdress(const uint32_t block_no) const { return block_size * block_no; }

	void getInodePosition(uint32_t* group_no, uint32_t* relative_inode_no, const uint32_t inode_no) const
	{
		if (inode_no % super_block.inodes_per_group == 0)
		{
			*group_no = (inode_no / super_block.inodes_per_group) - 1;
			*relative_inode_no = super_block.inodes_per_group - 1;
		}
		else {
			*group_no = (inode_no / super_block.inodes_per_group);
			*relative_inode_no = (inode_no % super_block.inodes_per_group) - 1;
		}
	}

	uint32_t getInodeAddress(const uint32_t inode_no) const
	{
		uint32_t group_no;
		uint32_t relative_inode_no;
		getInodePosition(&group_no, &relative_inode_no, inode_no);
		return getBlockAdress(block_groups[group_no].inode_table) + super_block.inode_size * relative_inode_no;
	}

	ext2_inode getInode(const uint32_t inode_no) const {
		ext2_inode retval;
		lseek(fd, getInodeAddress(inode_no), SEEK_SET);
		read(fd, &retval, sizeof(ext2_inode));
		return retval;
	}

	void getAllBlocks(const uint32_t block_no, vector<uint32_t>* blocks, const uint8_t level) const {
		uint32_t addr = getBlockAdress(block_no);
		for (uint32_t i = 0; i < block_size / sizeof(uint32_t); i++) {
			lseek(fd, addr + i * sizeof(uint32_t), SEEK_SET);
			uint32_t temp;
			read(fd, &temp, sizeof(uint32_t));
			if (temp != 0) {
				if (level == 1)
					blocks->push_back(temp);
				else
					getContentBlocks(temp, blocks, level - 1);
			}
		}
		blocks->push_back(block_no);
	}

	void getAllBlocks(const ext2_inode inode, vector<uint32_t>* blocks) const {
		for (uint32_t i = 0; i < EXT2_NUM_DIRECT_BLOCKS; i++) {
			if (inode.direct_blocks[i] != 0) {
				blocks->push_back(inode.direct_blocks[i]);
			}
		}
		if (inode.single_indirect != 0) {
			getAllBlocks(inode.single_indirect, blocks, 1);
		}
		if (inode.double_indirect != 0) {
			getAllBlocks(inode.double_indirect, blocks, 2);
		}
		if (inode.triple_indirect != 0) {
			getAllBlocks(inode.triple_indirect, blocks, 3);
		}
	}

	void getAllBlocks(const uint32_t inode_no, vector<uint32_t>* blocks) const {
		ext2_inode inode = getInode(inode_no);
		getAllBlocks(inode, blocks);
	}

	void getContentBlocks(const uint32_t block_no, vector<uint32_t>* blocks, const uint8_t level) const {
		uint32_t addr = getBlockAdress(block_no);
		for (uint32_t i = 0; i < block_size / sizeof(uint32_t); i++) {
			lseek(fd, addr + i * sizeof(uint32_t), SEEK_SET);
			uint32_t temp;
			read(fd, &temp, sizeof(uint32_t));
			if (temp != 0) {
				if (level == 1)
					blocks->push_back(temp);
				else
					getContentBlocks(temp, blocks, level - 1);
			}
		}
	}

	void getContentBlocks(const ext2_inode inode, vector<uint32_t>* blocks) const {
		for (uint32_t i = 0; i < EXT2_NUM_DIRECT_BLOCKS; i++) {
			if (inode.direct_blocks[i] != 0) {
				blocks->push_back(inode.direct_blocks[i]);
			}
		}
		if (inode.single_indirect != 0) {
			getContentBlocks(inode.single_indirect, blocks, 1);
		}
		if (inode.double_indirect != 0) {
			getContentBlocks(inode.double_indirect, blocks, 2);
		}
		if (inode.triple_indirect != 0) {
			getContentBlocks(inode.triple_indirect, blocks, 3);
		}
	}

	void getContentBlocks(const uint32_t inode_no, vector<uint32_t>* blocks) const {
		ext2_inode inode = getInode(inode_no);
		getContentBlocks(inode, blocks);
	}

	uint32_t readDirectoryBlock(const uint32_t block_no, const char* find) const {
		uint32_t base = getBlockAdress(block_no);
		unsigned char* block = new unsigned char[block_size];
		lseek(fd, base, SEEK_SET);
		read(fd, block, block_size);
		ext2_dir_entry* entry = (ext2_dir_entry*)block;
		unsigned int size = 0;

		while (size < block_size) {
			if (strncmp(entry->name, find, strlen(find)) == 0) {
				delete[] block;
				return entry->inode;
			}
			size += entry->length;
			entry = (ext2_dir_entry*)((char*)entry + entry->length);
		}
		delete[] block;
		return 0;
	}

	uint32_t findInDirectory(const uint32_t inode_no, const char* find) const {
		vector<uint32_t> blocks;
		getContentBlocks(inode_no, &blocks);
		uint32_t retval = 0;
		for (uint32_t i = 0; i < blocks.size(); i++) {
			retval = readDirectoryBlock(blocks[i], find);
			if (retval > 0) {
				return retval;
			}
		}
		return retval;
	}

	uint32_t getDirectory(const vector<string> tokenized_path, vector<uint32_t> *path_record) const {
		uint32_t retval = 2;
		if(path_record != NULL)
			path_record->push_back(retval);
		for (uint32_t i = 0; i < tokenized_path.size(); i++) {
			retval = findInDirectory(retval, tokenized_path[i].c_str());
			if (retval == 0) {
				return retval;
			}
			if (path_record != NULL)
				path_record->push_back(retval);
		}
		return retval;
	}

	uint32_t getDirectory(const char* path, vector<uint32_t>* path_record) const {
		vector<string> tokenized_path = tokenizePath(path);
		return getDirectory(tokenized_path, path_record);
	}

	void printDirectoryBlock(const uint32_t block_no) const {
		uint32_t base = getBlockAdress(block_no);
		unsigned char* block = new unsigned char[block_size];
		lseek(fd, base, SEEK_SET);
		read(fd, block, block_size);
		ext2_dir_entry* entry = (ext2_dir_entry*)block;
		unsigned int size = 0;

		while (size < block_size) {
			char* f = new char[entry->name_length + 1];
			strncpy(f, entry->name, entry->name_length);
			f[entry->name_length] = '\0';
			print_dir_entry(entry, f);
			size += entry->length;
			entry = (ext2_dir_entry*)((char*)entry + entry->length);
			delete[] f;
		}
		delete[] block;
	}

	void printDirectory(const uint32_t inode_no) const {
		vector<uint32_t> blocks;
		getContentBlocks(inode_no, &blocks);
		for (uint32_t i = 0; i < blocks.size(); i++) {
			printDirectoryBlock(blocks[i]);
		}
	}

	uint32_t findFreeInode() const {
		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			if (block_groups[i].free_inode_count > 0) {
				for (uint32_t j = 0; j < block_size; j++) {
					unsigned char temp = inode_bitmaps[i][j];
					if (temp < 255) {
						for (uint32_t k = 0; k < 8; k++)
						{
							if (temp & (unsigned char)0x1)
							{
								temp = (temp) >> 1;
							}
							else
							{
								if(super_block.inodes_per_group * i + 8 * j + k + 1 > super_block.first_inode)
									return super_block.inodes_per_group * i + 8 * j + k + 1;
							}
						}
					}
				}
			}
		}
		return uint32_t();
	}

	uint32_t findFreeBlock() const {
		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			if (block_groups[i].free_block_count > 0) {
				for (uint32_t j = 0; j < block_size; j++) {
					unsigned char temp = block_bitmaps[i][j];
					if (temp < 255) {
						for (uint32_t k = 0; k < 8; k++)
						{
							if (temp & (unsigned char)0x1)
							{
								temp = (temp) >> 1;
							}
							else
							{
								return super_block.blocks_per_group * i + 8 * j + k;
							}
						}
					}
				}
			}
		}
		return uint32_t();
	}

	void setInode(const uint32_t inode_no) {
		uint32_t group_no;
		uint32_t relative_inode_no;
		if (inode_no % super_block.inodes_per_group == 0)
		{
			group_no = (inode_no / super_block.inodes_per_group) - 1;
			relative_inode_no = super_block.inodes_per_group - 1;
		}
		else {
			group_no = (inode_no / super_block.inodes_per_group);
			relative_inode_no = (inode_no % super_block.inodes_per_group) - 1;
		}
		inode_bitmaps[group_no][relative_inode_no / 8] = inode_bitmaps[group_no][relative_inode_no / 8] ^ ((unsigned char)0x1 << (relative_inode_no % 8));
		block_groups[group_no].free_inode_count = block_groups[group_no].free_inode_count - 1;
		super_block.free_inode_count -= 1;
		cout << "EXT2_ALLOC_INODE " << inode_no << endl;
	}

	void setBlock(const uint32_t block_no) {
		uint32_t group_no = block_no / super_block.blocks_per_group;
		uint32_t relative_block_no = block_no % super_block.blocks_per_group;
		block_bitmaps[group_no][relative_block_no / 8] = block_bitmaps[group_no][relative_block_no / 8] ^ ((unsigned char)0x1 << (relative_block_no % 8));
		block_groups[group_no].free_block_count = block_groups[group_no].free_block_count - 1;
		super_block.free_block_count -= 1;
		cout << "EXT2_ALLOC_BLOCK " << block_no << endl;
	}

	void unsetInode(const uint32_t inode_no) {
		uint32_t group_no;
		uint32_t relative_inode_no;
		if (inode_no % super_block.inodes_per_group == 0)
		{
			group_no = (inode_no / super_block.inodes_per_group) - 1;
			relative_inode_no = super_block.inodes_per_group - 1;
		}
		else {
			group_no = (inode_no / super_block.inodes_per_group);
			relative_inode_no = (inode_no % super_block.inodes_per_group) - 1;
		}
		inode_bitmaps[group_no][relative_inode_no / 8] = inode_bitmaps[group_no][relative_inode_no / 8] ^ ((unsigned char)0x1 << (relative_inode_no % 8));
		block_groups[group_no].free_inode_count = block_groups[group_no].free_inode_count + 1;
		super_block.free_inode_count += 1;
		cout << "EXT2_FREE_INODE " << inode_no << endl;
	}

	void unsetBlock(const uint32_t block_no) {
		uint32_t group_no = block_no / super_block.blocks_per_group;
		uint32_t relative_block_no = block_no % super_block.blocks_per_group;
		block_bitmaps[group_no][relative_block_no / 8] = block_bitmaps[group_no][relative_block_no / 8] ^ ((unsigned char)0x1 << (relative_block_no % 8));
		block_groups[group_no].free_block_count = block_groups[group_no].free_block_count + 1;
		super_block.free_block_count += 1;
		cout << "EXT2_FREE_BLOCK " << block_no << endl;
	}

	bool hasEmptySpace(const uint32_t block_no, const uint32_t minSize) const {
		uint32_t base = getBlockAdress(block_no);
		unsigned char* block = new unsigned char[block_size];
		lseek(fd, base, SEEK_SET);
		read(fd, block, block_size);
		ext2_dir_entry* entry = (ext2_dir_entry*)block;
		uint32_t size = 0;
		while (size < block_size) {
			uint32_t true_size = EXT2_DIR_LENGTH(entry->name_length);
			if (entry->length - true_size > minSize) {
				delete[] block;
				return true;
			}
				
			size += entry->length;
			entry = (ext2_dir_entry*)((char*)entry + entry->length);
		}
		delete[] block;
		return false;
	}

	uint32_t getBlockWithFreeSpace(const uint32_t inode_no, const uint32_t minSize) const {
		vector<uint32_t> blocks;
		getContentBlocks(inode_no, &blocks);
		for (uint32_t i = 0; i < blocks.size(); i++) {
			if (hasEmptySpace(blocks[i], minSize))
				return blocks[i];
		}
		return 0;
	}

	void writeInode(ext2_inode inode, uint32_t inode_no) {
		lseek(fd, getInodeAddress(inode_no), SEEK_SET);
		write(fd, &inode, super_block.inode_size);
	}

	void updateInodeTime(uint32_t inode_no, uint32_t timeval) {
		ext2_inode inode_update = getInode(inode_no);
		inode_update.access_time = timeval;
		inode_update.modification_time = timeval;
		writeInode(inode_update, inode_no);
	}

	void saveBitmaps() {
		for (uint32_t i = 0; i < number_of_block_groups; i++) {

			lseek(fd, getBlockAdress(block_groups[i].block_bitmap), SEEK_SET);
			write(fd, block_bitmaps[i], block_size);


			lseek(fd, getBlockAdress(block_groups[i].inode_bitmap), SEEK_SET);
			write(fd, inode_bitmaps[i], block_size);
		}
	}

	void writeDirEntry(ext2_dir_entry entry, uint32_t address, const char* name) {
		lseek(fd, address, SEEK_SET);
		write(fd, &entry, sizeof(entry));
		write(fd, name, entry.name_length);
	}

	void saveBlockGroups() {
		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			if (block_size != EXT2_SUPER_BLOCK_SIZE) {
				lseek(fd, block_size, SEEK_SET);
				write(fd, block_groups, sizeof(ext2_block_group_descriptor) * number_of_block_groups);
			}
			else {
				lseek(fd, EXT2_SUPER_BLOCK_POSITION + EXT2_SUPER_BLOCK_SIZE, SEEK_SET);
				write(fd, block_groups, sizeof(ext2_block_group_descriptor) * number_of_block_groups);
			}
		}
	}

	uint32_t getDirBlockEntryCount(const uint32_t block_no) const {
		uint32_t retval = 0;
		uint32_t base = getBlockAdress(block_no);
		unsigned char* block = new unsigned char[block_size];
		lseek(fd, base, SEEK_SET);
		read(fd, block, block_size);
		ext2_dir_entry* entry = (ext2_dir_entry*)block;
		unsigned int size = 0;

		while (size < block_size) {
			retval++;
			size += entry->length;
			entry = (ext2_dir_entry*)((char*)entry + entry->length);
		}
		delete[] block;
		return retval;
	}

	uint32_t getDirEntryCount(const ext2_inode inode) const {
		vector<uint32_t> blocks;
		getContentBlocks(inode, &blocks);
		uint32_t retval = 0;
		for (uint32_t i = 0; i < blocks.size(); i++) {
			retval += getDirBlockEntryCount(blocks[i]);
		}
		return retval;
	}

	bool removeEntry(const uint32_t block_no, const char* dirEntryName) {
		uint32_t retval = 0;
		uint32_t base = getBlockAdress(block_no);
		unsigned char* block = new unsigned char[block_size];
		lseek(fd, base, SEEK_SET);
		read(fd, block, block_size);
		ext2_dir_entry* entry = (ext2_dir_entry*)block;
		ext2_dir_entry* entry2 = (ext2_dir_entry*)block;
		unsigned int size = 0;

		while (size < block_size) {
			char* f = new char[entry->name_length + 1];
			
			strncpy(f, entry->name, entry->name_length);
			f[entry->name_length] = '\0';

			if (strcmp(dirEntryName, f)) {
				delete[] f;
				char* f2 = new char[entry2->name_length + 1];
				f2[entry2->name_length] = '\0';
				entry2->length += entry->length;
				writeDirEntry(*entry2, base + size - entry2->length + entry->length, f2);
				delete[] f2;
				delete[] block;
				cout << "EXT2_FREE_ENTRY \"" << dirEntryName << "\" " << block_no << endl;
				return true;
			}
			size += entry->length;
			entry2 = entry;
			entry = (ext2_dir_entry*)((char*)entry + entry->length);
			delete[] f;
		}
		delete[] block;
		return false;
	}

	void removeDirEntry(const ext2_inode inode, const char* dirEntryName) {
		//TODO: Handling for if a block becomes empty after removal
		vector<uint32_t> blocks;
		getContentBlocks(inode, &blocks);
		for (uint32_t i = 0; i < blocks.size(); i++) {
			if (removeEntry(blocks[i], dirEntryName))
				return;
		}
	}

	void unsetAllDir(ext2_inode inode, const uint32_t inode_no, const uint32_t timeval) {
		unsetInode(inode_no);
		vector<uint32_t> blocks;
		getAllBlocks(inode, &blocks);
		for (uint32_t i = 0; i < blocks.size(); i++) {
			unsetBlock(blocks[i]);
		}
		inode.access_time = timeval;
		inode.modification_time = timeval;
		inode.deletion_time = timeval;
		inode.link_count -= 2;
		writeInode(inode, inode_no);
	}

	void unsetAllFile(ext2_inode inode, const uint32_t inode_no, const uint32_t timeval) {
		unsetInode(inode_no);
		vector<uint32_t> blocks;
		getAllBlocks(inode, &blocks);
		for (uint32_t i = 0; i < blocks.size(); i++) {
			unsetBlock(blocks[i]);
		}
		
		inode.access_time = timeval;
		inode.modification_time = timeval;
		inode.deletion_time = timeval;
		writeInode(inode, inode_no);
	}

	void ext2_mkdir(const char* path) {
		time_t t = time(0);

		vector<string> tokenized_path = tokenizePath(path);
		vector<string> parent_path(tokenized_path.begin(), tokenized_path.end() - 1);

		string name = tokenized_path[tokenized_path.size() - 1];
		vector<uint32_t> pathRecord;
		uint32_t parent_inode_no = getDirectory(parent_path, &pathRecord);
		if (parent_inode_no == 0) {
			return;
		}
		
		ext2_inode parent_inode = getInode(parent_inode_no);

		uint32_t free_inode = findFreeInode();
		uint32_t free_block = findFreeBlock();
		uint32_t timeval = (uint32_t)UPDATE_TIME(t);

		setInode(free_inode);
		writeInode(ext2_inode{ EXT2_I_DTYPE + EXT2_I_DPERM, EXT2_I_UID, block_size, timeval, timeval, timeval, 0, EXT2_I_GID, 2, CEIL(block_size, 512), 0, 0, {free_block}, 0, 0, 0}, free_inode);
		uint32_t group_no;
		uint32_t rel;
		getInodePosition(&group_no, &rel, free_inode);
		block_groups[group_no].used_dirs_count += 1;
		
		
		setBlock(free_block);
		
		uint32_t minSize = EXT2_DIR_LENGTH(name.size());
		uint32_t parent_directory = getBlockWithFreeSpace(parent_inode_no, minSize);

		if (parent_directory != 0) {
			uint32_t base = getBlockAdress(parent_directory);
			unsigned char* block = new unsigned char[block_size];
			lseek(fd, base, SEEK_SET);
			read(fd, block, block_size);
			ext2_dir_entry* entry = (ext2_dir_entry*)block;
			uint32_t size = 0;
			while (size < block_size) {
				uint32_t true_size = EXT2_DIR_LENGTH(entry->name_length);
				if (entry->length - true_size > minSize) {

					char* entry_name = new char[entry->name_length];
					lseek(fd, base + size + sizeof(ext2_dir_entry), SEEK_SET);
					read(fd, entry_name, entry->name_length);

					writeDirEntry(ext2_dir_entry{ entry->inode, (uint16_t)true_size,  entry->name_length, entry->file_type }, base + size, entry_name);
					writeDirEntry(ext2_dir_entry{ free_inode, (uint16_t)(entry->length - true_size), (uint8_t)name.size(), EXT2_D_DTYPE }, base + size + true_size, name.c_str());
					cout << "EXT2_ALLOC_ENTRY " << "\"" <<name << "\" " << parent_directory << endl;
					delete[] entry_name;
					break;
				}

				size += entry->length;
				entry = (ext2_dir_entry*)((char*)entry + entry->length);
			}
			delete[] block;
		}
		else {
			//TODO: allocate new block to extend parent directory
		}

		writeDirEntry(ext2_dir_entry{ free_inode, EXT2_DIR_LENGTH(1), 1, EXT2_D_DTYPE }, getBlockAdress(free_block), ".");
		cout << "EXT2_ALLOC_ENTRY " << "\".\" " << free_block << endl;
		
		writeDirEntry(ext2_dir_entry{ parent_inode_no, (uint16_t)(block_size - EXT2_DIR_LENGTH(1)), 2, EXT2_D_DTYPE }, getBlockAdress(free_block) + EXT2_DIR_LENGTH(1), "..");
		cout << "EXT2_ALLOC_ENTRY " << "\"..\" " << free_block << endl;

		parent_inode.link_count += 1;
		writeInode(parent_inode, parent_inode_no);

		for (uint32_t i = 0; i < pathRecord.size(); i++) {
			updateInodeTime(pathRecord[i], timeval);
		}
	}

	void ext2_rmdir(const char* path) {
		time_t t = time(0);
		uint32_t timeval = UPDATE_TIME(t);
		vector<string> tokenized_path = tokenizePath(path);
		string name = tokenized_path[tokenized_path.size() - 1];
		vector<uint32_t> pathRecord;
		uint32_t inode_no = getDirectory(tokenized_path, &pathRecord);
		if (inode_no == 0) {
			return;
		}
		ext2_inode inode = getInode(inode_no);
		if (getDirEntryCount(inode) > 2) {
			return;
		}
		ext2_inode parent_inode = getInode(pathRecord[pathRecord.size() - 2]);
		removeDirEntry(parent_inode, name.c_str());
		parent_inode.link_count -= 1;
		writeInode(parent_inode, pathRecord[pathRecord.size() - 2]);
		unsetAllDir(inode, inode_no, timeval);
		uint32_t group_no;
		uint32_t rel;
		getInodePosition(&group_no, &rel, inode_no);
		block_groups[group_no].used_dirs_count -= 1;
		for (uint32_t i = 0; i < pathRecord.size(); i++) {
			updateInodeTime(pathRecord[i], timeval);
		}
	}

	void ext2_rm(const char* path) {
		time_t t = time(0);
		uint32_t timeval = UPDATE_TIME(t);
		vector<string> tokenized_path = tokenizePath(path);
		string name = tokenized_path[tokenized_path.size() - 1];
		vector<uint32_t> pathRecord;
		uint32_t inode_no = getDirectory(tokenized_path, &pathRecord);
		if (inode_no == 0) {
			return;
		}
		ext2_inode parent_inode = getInode(pathRecord[pathRecord.size() - 2]);
		removeDirEntry(parent_inode, name.c_str());
		
		writeInode(parent_inode, pathRecord[pathRecord.size() - 2]);
		ext2_inode inode = getInode(inode_no);
		if (inode.link_count > 1)
			inode.link_count -= 1;
		else
			inode.link_count = 0;
		writeInode(inode, inode_no);
		if (inode.link_count == 0) {
			unsetAllFile(inode, inode_no, timeval);
		}
		for (uint32_t i = 0; i < pathRecord.size(); i++) {
			updateInodeTime(pathRecord[i], timeval);
		}
	}

	void ext2_ed(const char* path, const string content, const uint32_t index, const uint32_t bspace) {
		time_t t = time(0);
		uint32_t timeval = UPDATE_TIME(t);
		vector<string> tokenized_path = tokenizePath(path);
		string name = tokenized_path[tokenized_path.size() - 1];
		vector<uint32_t> pathRecord;
		uint32_t inode_no = getDirectory(tokenized_path, &pathRecord);
		if (inode_no == 0) {
			return;
		}
		ext2_inode inode = getInode(inode_no);
		ext2_inode parent_inode = getInode(pathRecord[pathRecord.size() - 2]);
		char* content_buffer = new char[inode.size - index + content.size()];

		for (uint32_t i = 0; i < pathRecord.size(); i++) {
			updateInodeTime(pathRecord[i], timeval);
		}
		delete[] content_buffer;
	}

	filesystem(const char* filesystem_path) {
		fd = open(filesystem_path, O_RDWR);
		lseek(fd, EXT2_SUPER_BLOCK_POSITION, SEEK_SET);
		read(fd, &(super_block), sizeof(ext2_super_block));

		block_size = EXT2_UNLOG(super_block.log_block_size);
		number_of_block_groups = CEIL(super_block.inode_count, super_block.inodes_per_group);

		block_groups = new ext2_block_group_descriptor[number_of_block_groups];

		if (block_size != EXT2_SUPER_BLOCK_SIZE) {
			lseek(fd, block_size, SEEK_SET);
			read(fd, block_groups, sizeof(ext2_block_group_descriptor) * number_of_block_groups);
		}
		else {
			lseek(fd, EXT2_SUPER_BLOCK_POSITION + EXT2_SUPER_BLOCK_SIZE, SEEK_SET);
			read(fd, block_groups, sizeof(ext2_block_group_descriptor) * number_of_block_groups);
		}

		root_inode = getInode(2);

		block_bitmaps = new unsigned char* [number_of_block_groups];
		inode_bitmaps = new unsigned char* [number_of_block_groups];

		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			block_bitmaps[i] = new unsigned char[block_size];
			lseek(fd, getBlockAdress(block_groups[i].block_bitmap), SEEK_SET);
			read(fd, block_bitmaps[i], block_size);

			inode_bitmaps[i] = new unsigned char[block_size];
			lseek(fd, getBlockAdress(block_groups[i].inode_bitmap), SEEK_SET);
			read(fd, inode_bitmaps[i], block_size);
		}
	}

	void print_system() const {
		print_super_block(&super_block);
		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			print_group_descriptor(block_groups + i);
		}
		print_inode(&root_inode, 2);
	}

	void print_bitmaps(const bitmap_type type, const uint32_t block_group_no) const {
		for (uint32_t j = 0; j < block_size; j++) {
			if (type == bitmap_type::INODE_BITMAP)
				cout << hex << (int)inode_bitmaps[block_group_no][j] << " ";
			else
				cout << hex << (int)block_bitmaps[block_group_no][j] << " ";
		}
		cout << endl;
	}

	void print_bitmaps(const bitmap_type type) const {
		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			for (uint32_t j = 0; j < block_size; j++) {
				if (type == bitmap_type::INODE_BITMAP)
					cout << hex << (int)inode_bitmaps[i][j] << " ";
				else
					cout << hex << (int)block_bitmaps[i][j] << " ";
			}
			cout << endl;
		}
	}

	void destroy_system() {
		saveBitmaps();
		saveBlockGroups();
		lseek(fd, EXT2_SUPER_BLOCK_POSITION, SEEK_SET);
		write(fd, &(super_block), sizeof(ext2_super_block));
		close(fd);
		delete[] block_groups;
		for (uint32_t i = 0; i < number_of_block_groups; i++) {
			delete[] block_bitmaps[i];
			delete[] inode_bitmaps[i];
		}
		delete[] block_bitmaps;
		delete[] inode_bitmaps;
	}
};

