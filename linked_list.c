#include <stdio.h>
#include <stdlib.h>
/*
typedef struct node
{
    int data;
    struct node * next;
}linklist;

void sortlist(linklist * head)
{
    linklist * newhead, * s, * pre ,* p;
    p=head->next;
    newhead=p->next;
    p->next=NULL;
    while(newhead)
    {
        s=newhead;
        newhead=newhead->next;
        pre=head;
        p=head->next;
        while(p!=NULL && p->data < s->data)
        {
            pre=p;
            p=p->next;
        }
        s->next=p;
        pre->next=s;
    }
}

int main()
{
    linklist * head = NULL;

    head = malloc ( sizeof ( linklist ) );
    head->data = 33;
    head->next = NULL;

    linklist * new_node = malloc ( sizeof ( linklist ) );
    new_node->data = 22;
    new_node->next = NULL;

    head->next = new_node;
    
    sortlist ( head );

    while ( head!= NULL )
        printf ( "%d\n", head->data );

    return 0;
}

*/




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

