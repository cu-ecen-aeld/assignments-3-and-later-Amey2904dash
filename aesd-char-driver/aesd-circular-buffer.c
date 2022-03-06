/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer. 
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
			size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    uint8_t index = buffer->out_offs;
    size_t sum_val = 0, last_size_val = 0 , diff_val = 0;
    
    do
    {
    	// Calculate the sum and index values
        sum_val += buffer->entry[index].size; //increment the sum
        last_size_val = buffer->entry[index].size;	// calculate the last size entry value
        
        if(char_offset <= (sum_val-1))  //if found in 1st block
        {
            diff_val = (sum_val - last_size_val);
            *entry_offset_byte_rtn = char_offset - diff_val;
            return &buffer->entry[index]; // return
        }
        else
        {
	 	index++; //increment the index
	 	
		if(index == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) // if index == 10
		{
		    index = 0; // reset the index
		}
        }
    } while(index != buffer->in_offs);
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description 
    */
    
   if(buffer->full == true) // check if buffer is full 
   {
        buffer->entry[buffer->in_offs] = *(add_entry); //add string and size overwrite data to the buffer
        buffer->in_offs++; //incremnet the tail
        buffer->out_offs++; //incremnet the head
        return;
   }
   else
   {
    buffer->entry[buffer->in_offs] = *(add_entry); //add string and size data to the buffer
    buffer->in_offs++; //increment the head

    if(buffer->in_offs == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED) //If tail reaches end (10), roll back (reset tail to first)
    { 
        buffer->in_offs = 0;
    }

    if(buffer->in_offs == buffer->out_offs)  //Check if full, i.e out_offs == in_offs , head == tail
    { 
        buffer->full = true;
    }
    else
    {
        buffer->full = false;
    }
  }
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
