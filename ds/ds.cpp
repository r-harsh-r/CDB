#include "./ds.h"

DiskFile::DiskFile(const char*filename_) : filename(filename_){
    fd = open(filename,O_RDWR|O_CREAT,0644);
    if(fd < 0){
        perror("Open failed");
        exit(1);
    }

    // if these information is already on the location, then copy it from there else default value
    off_t fsize = lseek(fd, 0, SEEK_END);
    
    if (fsize == 0) {
        // New file: Initialize header
        char buff[PAGE_SIZE] = {0};
        lseek(fd,0,SEEK_SET);
        write(fd,buff,sizeof(buff));

        page_size = PAGE_SIZE;
        next_free_page = 0;
        total_page_alloted = 1;
        root_page = 0;
        updateHeader();
    } else {
        // Existing file: Read header
        lseek(fd, 0, SEEK_SET);
        read(fd, &page_size, sizeof(page_size));
        read(fd, &next_free_page, sizeof(next_free_page));
        read(fd, &total_page_alloted, sizeof(total_page_alloted));
        read(fd, &root_page, sizeof(root_page));
    }

}

DiskFile :: ~DiskFile(){
    if(fd >= 0){
        close(fd);
    }
}

void DiskFile ::updateHeader(){
    lseek(fd,0,SEEK_SET);
    write(fd,&page_size,sizeof(page_size));
    
    write(fd,&next_free_page,sizeof next_free_page);
    
    write(fd,&total_page_alloted,sizeof(total_page_alloted));
    
    write(fd,&root_page,sizeof(root_page));
}

bool DiskFile :: readPage(uint32_t pageNum,void*buff){
    off_t offset = lseek(fd,pageNum*PAGE_SIZE,SEEK_SET);
    if(offset == -1){
        return false;
    }
    ssize_t n = read(fd,buff,PAGE_SIZE);
    return n == PAGE_SIZE;
}
