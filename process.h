class Process {
	private:
		int pid;
		int startTime;
		int duration;
        bool started;

	public:
		Process(int, int);
		int getStartTime();
		int getDuration();
		int getPid();
        void start();
        bool isRunning();
};
