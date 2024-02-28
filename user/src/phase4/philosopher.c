#include "philosopher.h"

// TODO: define some sem if you need
int f[PHI_NUM];

void init() {
  // init some sem if you need
  //TODO();
  for (int i = 0; i < PHI_NUM; i++) {
    f[i] = sem_open(1);
  }
}

void philosopher(int id) {
  // implement philosopher, remember to call `eat` and `think`
  while (1) {
    //TODO();
    think(id);
    if (id % 2 == 1) {//避免死锁，奇数编号的人先拿左手，偶数编号先拿右手，就不会出现每个人都拿了一个，剩下一个没有资源造成死锁
      P(f[id]);
      P(f[(id + 1) % PHI_NUM]);
    }
    else {
      P(f[(id + 1) % PHI_NUM]);
      P(f[id]);
    }
    eat(id);
    V(f[id]);
    V(f[(id + 1) % PHI_NUM]);
  }
}
