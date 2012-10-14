#include <stdio.h>
#include <stdlib.h>

struct LIST
{
  struct LIST * pNext;
  int           iValue;
};
 
struct LIST * SortList(struct LIST * pList)
{
  /* build up the sorted array from the empty list */
  struct LIST * pSorted = NULL;
 
  /* take items off the input list one by one until empty */
  while (pList != NULL)
    {
      /* remember the head */
      struct LIST *   pHead  = pList;
      /* trailing pointer for efficient splice */
      struct LIST ** ppTrail = &pSorted;
 
      /* pop head off list */
      pList = pList->pNext;
 
      /* splice head into sorted list at proper place */
      while (1)
        {
          /* does head belong here? */
          if (*ppTrail == NULL || pHead->iValue < (*ppTrail)->iValue)
            {
              /* yes */
              pHead->pNext = *ppTrail;
              *ppTrail = pHead;
              break;
            }
          else
            {
              /* no - continue down the list */
              ppTrail = & (*ppTrail)->pNext;
            }
        }
    }
 
  return pSorted;
}


int main()
{
    struct LIST * head = NULL;

    head = malloc ( sizeof ( struct LIST ) );
    head->iValue = 33;
    head->pNext = NULL;

    struct LIST * new_node = malloc ( sizeof ( struct LIST ) );
    new_node->iValue = 22;
    new_node->pNext = NULL;

    head->pNext = new_node;
    
    head = SortList ( head );

    struct LIST * ptr = head;

    while ( ptr!= NULL )
    {
        printf ( "%d\n", ptr->iValue );
        ptr = ptr->pNext;
    }
    return 0;
}

