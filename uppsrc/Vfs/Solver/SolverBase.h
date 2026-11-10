#ifndef _Vfs_Solver_SolverBase_h_
#define _Vfs_Solver_SolverBase_h_


#define PROCESS_ASSERT(x)  if (!(x)) {SetError("assert failed: " #x); SetNotRunning(); RLOG("PROCESS ASSERT: " << __FILE__ << ":" << __LINE__ << ": error: assert failed: " #x); SetWaiting(0); return;}
#define PROCESS_ASSERT_(x, str)  if (!(x)) {SetError("assert failed: " + (String)(str) + " (" #x ")"); SetNotRunning(); RLOG("PROCESS ASSERT: " << __FILE__ << ":" << __LINE__ << ": error: assert failed: " #x); SetWaiting(0); return;}
#define PROCESS_ASSERT_CMP(a,b)  if (!(a == b)) {String s = #a " == " #b": " + IntStr(a) + " != " + IntStr(b); SetError("assert failed: " + s); SetNotRunning(); RLOG("ScriptTextProcess: " << __FILE__ << ":" << __LINE__ << ": error: assert failed: " << s); SetWaiting(0); return;}

class SolverBase : public Pte<SolverBase> {
	
protected:
	struct WorkerData : Moveable<WorkerData> {
		int phase = 0, batch = 0, sub_batch = 0;
	};
	Vector<WorkerData> worker_data;
	static int& WorkerId();
	WorkerData& Worker();
	Mutex task_lock, data_lock;
	DatasetPtrs p;
	
	Time time_started, time_stopped;
	int generation = 0, phase = 0, batch = 0, sub_batch = 0, batch_count = 0;
	int generation_count = 1;
	
	bool waiting = false;
	bool running = false, stopped = true;
	bool skip_ready = true;
	bool parallel = false;
	String last_error;
	
	void Process();
	void ProcessInParallel();
	void ProcessInOrder();
	void ParallelWorker(int id);
	
	void PostProgress();
	void PostRemaining();
	void SetNotRunning();
	void SetWaiting(bool b);
	void SetGenerations(int i);
	void MovePhase(int p);
	void NextPhase();
	void NextBatch();
	void NextSubBatch();
	
	double GetProgress();
	void SetError(String s) {last_error = s;}
public:
	typedef SolverBase CLASSNAME;
	SolverBase();
	
	//TextDatabase& GetDatabase() const;
	int GetTypeclassCount() {TODO return -1; /*return TextLib::GetTypeclassCount(appmode);*/} // should be based on text files
	int GetContentCount() {TODO return -1; /*return TextLib::GetContentCount(appmode);*/} // should be based on text files
	
	void SetParallel(bool b=true) {parallel = b;}
	void Fail(String e, String title="");
	
	void Start() {if (!running) {running = true; stopped = false; Thread::Start(THISBACK(Process));}}
	void Stop() {running = false; while (!stopped) Sleep(1);}
	void SetSkipReady(bool b) {skip_ready = false;}
	static void StopAll();
	
	bool IsPhase(int i) const {return phase == i;}
	bool IsPhaseRange(int a, int b) const {return phase >= a && phase <= b;}
	bool IsRunning() const {return running;}
	bool IsPhaseInit() const {return batch == 0 && sub_batch == 0;}
	
	virtual int GetPhaseCount() const = 0;
	virtual int GetBatchCount(int phase) const {return max(1, batch);}
	virtual int GetSubBatchCount(int phase, int batch) const {return max(1, sub_batch);}
	virtual void DoPhase() = 0;
	virtual void OnBatchError() {NextBatch();}
	
	Callback2<int,int> WhenProgress;
	Callback1<String> WhenRemaining;
	Event<> WhenReady;
	Event<> WhenStopped;
	Event<String> WhenError;
	
};


#endif
