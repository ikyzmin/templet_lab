/*$TET$header*/
#define TET_DEBUG_SERIAL
#include <templet.hpp>
/*$TET$*/
#include <queue>

#ifndef TET_MPI_EXEC
#include <thread>
#endif

using namespace std;
using namespace TEMPLET;

struct task_result : message{

	task_result(engine*e,actor*m,actor*w): _master(m), _worker(w) {
		::init(this, e, save_adapter, restore_adapter);
		_call = FIRST_CALL;
	}

	void save(saver*s){
		::save(s, &_call, sizeof(_call));
		if(_call == NEXT_CALL) _result.save(s);
		else if(_call == REPLY) _task.save(s);
	}

	void restore(restorer*r){
		::restore(r,&_call,sizeof(_call));
		if(_call == NEXT_CALL) _result.restore(r);
		else if(_call == REPLY) _task.restore(r);
	}

	struct task{
		void save(saver*s){
/*$TET$task$save*/
/*$TET$*/
		}
		void restore(restorer*r){
/*$TET$task$restore*/
/*$TET$*/
		}
/*$TET$task$data*/
/*$TET$*/
	} _task;

	struct result{
		void save(saver*s){
/*$TET$result$save*/
/*$TET$*/
		}
		void restore(restorer*r){
/*$TET$result$restore*/
/*$TET$*/
		}
/*$TET$result$data*/
/*$TET$*/
	} _result;
	
	void call(){ _call = (_call == REPLY ? NEXT_CALL : FIRST_CALL);  TEMPLET::send(this, _master, _call); }
	void reply(){ _call = REPLY; TEMPLET::send(this, _worker, _call); }

	enum { FIRST_CALL, NEXT_CALL, REPLY } _call;
	actor* _master;
	actor* _worker;

	friend void save_adapter(message*m, saver*s){((task_result*)m)->save(s);}
	friend void restore_adapter(message*m, restorer*r){((task_result*)m)->restore(r);}
};

struct worker;

struct bag : actor{

	bag(int argc, char *argv[], int nproc = 0);
	~bag();
	void delay(double t){ TEMPLET::delay(this,t); }
	double speedup(){
		double T1, Tp, Smax, Sp; int Pmax;
		if (TEMPLET::stat(&_engine, &T1, &Tp, &Pmax, &Smax, 1, &Sp)) return Smax; else return -1;
	}

	void run(){	::map(&_engine); ::run(&_engine); }

	friend void recv_master(actor*a, message*m, int tag){((bag*)a)->recv((task_result*)m, tag);}

	queue<task_result*> _wait;
	int _active;

	void recv(task_result*m, int tag){
		_active--;
		if (tag == task_result::FIRST_CALL){
			if (get(&m->_task)){
				m->reply();_active++;
			}
			else{
				_wait.push(m);
			}
		}
		else if (tag == task_result::NEXT_CALL){
			put(&m->_result);
			_wait.push(m);

			while (!_wait.empty() && (m = _wait.front()) && get(&m->_task)){
				_wait.pop(); m->reply(); _active++;
			}

			if (!_active)TEMPLET::stop(this);
		}
	}

	bool get(task_result::task*t){
/*$TET$bag$get*/
/*$TET$*/	
	}

	void put(task_result::result*r){
/*$TET$bag$put*/
/*$TET$*/	
	}
	
	void save(saver*s){
/*$TET$bag$save*/
/*$TET$*/
	}
	
	void restore(restorer*r){
/*$TET$bag$restore*/
/*$TET$*/
	}
	
/*$TET$bag$data*/
/*$TET$*/

	engine _engine;
	task_result** _messages;
	worker** _workers;
	int _nworkers;
};

struct worker : actor{
public:
	worker(engine*e,bag*b):_bag(b){
		::init(this, e, recv_worker, save_adapter, restore_adapter);
		_init = true;
	}
	void delay(double t){ TEMPLET::delay(this,t); }

	void proc(task_result::task*t, task_result::result*r)
	{
/*$TET$proc$data*/
/*$TET$*/
	}

	friend void recv_worker (actor*a, message*m,int tag){
		task_result* tr = (task_result*)m;
		worker* w = (worker*)a;
		w->proc(&tr->_task,&tr->_result);
		tr->call();
	}

	void save(saver*s){if(_init)_bag->save(s); _init=false;}
	void restore(restorer*r){if(_init)_bag->restore(r); _init=false;}
	friend void save_adapter(actor*a, saver*s){((worker*)a)->save(s);}
	friend void restore_adapter(actor*a, restorer*r){((worker*)a)->restore(r);}

	bag* _bag;
	bool _init;
};

inline bag::bag(int argc, char *argv[], int nproc) {
	::init(&_engine, argc, argv);
	::init(this, &_engine, recv_master);

	if (nproc) _nworkers = nproc;
	else{
#ifdef TET_MPI_EXEC
		_nworkers = ::nodes(this);
#else 
		_nworkers = std::thread::hardware_concurrency();
#endif
	}

	_messages = new task_result*[_nworkers];
	_workers = new worker*[_nworkers];

	for (int i = 0; i < _nworkers; i++){
		_workers[i] = new worker(&_engine,this);
		_messages[i] = new task_result(&_engine,this,_workers[i]);
	}

#ifdef TET_MPI_EXEC
	::at(this, 0);
	for (int i = 0; i < _nworkers; i++)	::at(_workers[i], i);
#endif

	for (int i = 0; i < _nworkers; i++)_messages[i]->call();
	_active = _nworkers;
}

inline bag::~bag(){
	for (int i = 0; i < _nworkers; i++){delete _workers[i]; delete _messages[i];}
	delete[] _messages;
	delete[] _workers;
}

/*$TET$footer*/
/*$TET$*/
