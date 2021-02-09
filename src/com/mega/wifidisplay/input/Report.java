package com.mega.wifidisplay.input;

import java.nio.ByteBuffer;
import java.util.Queue;

public interface Report {
	byte[] build();
	Queue<ByteBuffer> getReportQueue();
	Class getHidDescription();
}
