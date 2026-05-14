#include "posting.hpp"
#include "file.hpp"


void PostingList::lock_and_insert(off_t /*location*/)
{
    seeker_block = reinterpret_cast<Posting_Block*>(file->cache.read_block(root_location));
    seeker_location = root_location;

    //loop thru the blocks until we find a block with space
    while(seeker_block->size == 508 && seeker_block->free_index == -1 && seeker_block->next != 0)
    {
        off_t next_seeker_location = seeker_block->next;
        seeker_block = file->load_posting_block(next_seeker_location);
        seeker_location = next_seeker_location;
    }

    //if we found a block with space, insert the location
    if (seeker_block->size < 508 || seeker_block->free_index != -1)
    {

    }
    //if we reached the end of the list, we need to create a new block
    else
    {
        off_t new_block_location = file->alloc_block();

        Posting_Block* new_block = file->load_posting_block(new_block_location);

        new_block->size = 1;
       // new_block->locations[0] = location;
        //seeker_block->next = file->cache.write_block(new_block);
        //file->cache.mark_dirty(seeker_location);
    }




}
