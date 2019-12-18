# TaskQueue
Implementing a  task queue. The task queue can have one publishers and multiple subscribers. A publisher can publish muliple task. Each subscriber can pull one task. Users of TaskQueue need to implement two abstract classes.
To publish tasks, the user needs to implement Publisher::operator().
To subscribe to task, the user needs to implement Subscriber::operator().
