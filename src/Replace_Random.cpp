#include "../include/ReplaceAlgorithm.h"
#include "../include/setting.h"
#include <stdlib.h>

static void init()
{
    srand(2023);
}

static void update(BCB *bcb_ptr, int is_free_frame)
{
    ;
}

static BCB *select_victim()
{
    BCB *bcb_ptr;
    // bcb_ptr = &buf_bcb[0];
    bcb_ptr = &buf_bcb[rand() % DEFBUFSIZE];
    return bcb_ptr;
}

const struct replace_alg Random_replace_alg = {
    .name = "random",
    .init = init,
    .update = update,
    .select_victim = select_victim,
};
