#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <utility>
using namespace std;
#include "actor.h"
#include "spawn.h"

#define COUNT 5000000



class Printer : public Actor<Printer>{
public:
  enum MessageType { MSG };
  typedef Message<Printer, MSG, int> Msg;
  void run(){
    //for (int i=1; i<=COUNT; i++){
      Msg* m = (Msg*)receive();
      printf("Got %d\n", m->payload);
      delete m;
      //}
  }
};


/*
class Incrementor : public Actor<Incrementor>{
public:
  enum MessageType {MSG};
  typedef Message<Incrementor, MSG, pair<int, int> > Msg;
  void run(){
    while (1){
      Msg* msg = (Msg*)receive();
      pair<int, int> p = msg->payload;
      p.first += 2;
      p.second--;
      delete msg;
      if (p.second == 0){
        final->send(new Printer::Msg(p.first));
      }else{
        next->send(new Incrementor::Msg(p));
      }
    }
  }
  Actor<Incrementor>* next;
  Actor<Printer>* final;
};*/


Printer::Msg* finalmsg = new Printer::Msg;
class Incrementor : public Actor<Incrementor>{
public:
  enum MessageType {MSG};
  typedef Message<Incrementor, MSG, pair<int, int> > Msg;
  __attribute__((always_inline)) void run(){
    while (1){
      Msg* msg = (Msg*)receive();
      pair<int, int> p = msg->payload;
      p.first += 2;
      p.second--;
      msg->payload = p;
      if (p.second == 0){
        finalmsg->payload = p.first;
        final->send(finalmsg);
      }else{
        next->send(msg);
      }
    }
  }
  Actor<Incrementor>* next;
  Actor<Printer>* final;
};




template <class ActorT>
int actor_thunk(ActorT* act){
  act->run();
  return 0;
}

template <class ActorT>
__attribute__((always_inline)) void actor_thunk_v(ActorT* act){
  act->run();
}



void spawn_incrementors(int n, int msg){
  Incrementor* incs = new Incrementor[n];
  Printer* p = new Printer;
  fiber_handle<int> pt = spawn_fiber_fixed<int, Printer*, actor_thunk<Printer> >(p, 102400);
#define SS 128
  char* stacks = (char*)new void*[n * (SS / sizeof(void*))];
  for (int i=0; i<n; i++){
    incs[i].next = i == n-1 ? &incs[0] : &incs[i+1];
    incs[i].final = p;
    spawn_fiber_fixed_detached<Incrementor*, actor_thunk_v<Incrementor> >(&incs[i], SS, (void*)&stacks[i*SS]);
  }
  incs[0].send(new Incrementor::Msg(make_pair(0, msg)));
  pt.join();
}




int main(){
  worker::spawn_workers(1);
  const int n = 10000000;
  //spawn_incrementors(n, n*100);
  spawn_incrementors(503, 50000000);
  /*
  FloatPrinter* fp = new FloatPrinter;

  int (*func)(FloatPrinter*) = actor_thunk<FloatPrinter>;

  FloatSender* fs = new FloatSender(fp);
  fiber_handle<int> fpt = spawn_fiber_fixed<int, FloatPrinter*, actor_thunk<FloatPrinter> >(fp, 1024*3);
  fiber_handle<int> fst = spawn_fiber_fixed<int, FloatSender*, actor_thunk<FloatSender> >(fs, 10543);


  fpt.join();
  fst.join();
  */
  worker::await_completion();
}
