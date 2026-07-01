#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int *arr;
    size_t size;
    size_t cap;
} IntVec;

static void vec_init(IntVec *v)
{
    v->arr = NULL;
    v->size = 0;
    v->cap = 0;
}

static void vec_free(IntVec *v)
{
    free(v->arr);
    v->arr = NULL;
    v->size = 0;
    v->cap = 0;
}

static int vec_push_back(IntVec *v, int x)
{
    if (v->size == v->cap)
    {
        size_t new_cap = (v->cap == 0) ? 1 : (v->cap * 2);
        int *new_arr = (int *)realloc(v->arr, new_cap * sizeof(int));
        if (new_arr == NULL)
        {
            return 0;
        }
        v->arr = new_arr;
        v->cap = new_cap;
    }

    v->arr[v->size] = x;
    v->size++;
    return 1;
}

int main(void)
{
    IntVec v;
    vec_init(&v);

    for (int i = 0; i < 10; i++)
    {
        if (!vec_push_back(&v, i * i))
        {
            fprintf(stderr, "realloc failed\n");
            vec_free(&v);
            return 1;
        }
    }

    printf("size=%zu cap=%zu last=%d\n", v.size, v.cap, v.arr[v.size - 1]);

    vec_free(&v);
    return 0;
}
