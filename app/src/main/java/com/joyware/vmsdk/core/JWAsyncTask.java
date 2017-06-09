package com.joyware.vmsdk.core;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.MainThread;
import android.support.annotation.WorkerThread;

/**
 * Created by zhourui on 17/5/11.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public abstract class JWAsyncTask<Params, Progress, Result>  {

    private static final int MESSAGE_POST_PROGRESS = 1;
    private static final int MESSAGE_POST_FINISH = 2;

    private Thread mBackgroundThread;

    private Handler mMainHandler;

    private boolean mCancelled;

    @MainThread
    public final JWAsyncTask<Params, Progress, Result> execute(final Params... params) {
        if (mBackgroundThread != null) {
            throw new IllegalStateException("Cannot execute task:"
                    + " the task is already running.");
        }

        mMainHandler = new MyMainHandler(Looper.getMainLooper());

        onPreExecute();

        mBackgroundThread = new Thread(new Runnable() {

            @Override
            public void run() {
                Result result = null;
                try {
                    result = doInBackground(params);
                } catch (InterruptedException e) {

                } finally {
                    if (mMainHandler != null) {
                        Message message = Message.obtain();
                        message.what = MESSAGE_POST_FINISH;
                        message.obj = new JWAsyncTaskResult<Result>(JWAsyncTask.this, result);
                        mMainHandler.sendMessage(message);
                    }
                }
            }
        });

        mBackgroundThread.start();

        return this;
    }

    private void finish(Result result) {
        if (isCancelled()) {
            onCancelled(result);
        } else {
            onPostExecute(result);
        }
    }

    @MainThread
    protected void onPostExecute(Result result) {

    }

    public final boolean cancel(boolean mayInterruptIfRunning) {
        synchronized (this) {
            if (mBackgroundThread == null) {
                return false;
            }
            mCancelled = true;
            if (mayInterruptIfRunning) {
                mBackgroundThread.interrupt();
                try {
                    mBackgroundThread.join();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
        return true;
    }

    @WorkerThread
    protected final void publishProgress(Progress... values) {
        if (!isCancelled()) {
            Message message = Message.obtain();
            message.what = MESSAGE_POST_PROGRESS;
            message.obj = new JWAsyncTaskResult<Progress>(this, values);
            mMainHandler.sendMessage(message);
        }
    }

    @WorkerThread
    protected abstract Result doInBackground(Params... params) throws InterruptedException;

    @MainThread
    protected void onPreExecute() {

    }

    @SuppressWarnings({"UnusedDeclaration"})
    @MainThread
    protected void onProgressUpdate(Progress... values) {
    }

    @SuppressWarnings({"UnusedParameters"})
    @MainThread
    protected void onCancelled(Result result) {
        onCancelled();
    }

    @MainThread
    protected void onCancelled() {

    }

    public boolean isCancelled() {
        return mCancelled;
    }

    private static class MyMainHandler extends Handler {

        public MyMainHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MESSAGE_POST_PROGRESS:
                    JWAsyncTaskResult progress = (JWAsyncTaskResult) msg.obj;
                    progress.mTask.onProgressUpdate(progress.mData);
                    break;
                case MESSAGE_POST_FINISH:
                    JWAsyncTaskResult result = (JWAsyncTaskResult) msg.obj;
                    result.mTask.finish(result.mData[0]);
                    result.mTask.mCancelled = true;
                    break;
            }
        }
    }

    @SuppressWarnings({"RawUseOfParameterizedType"})
    private static class JWAsyncTaskResult<Data> {
        final JWAsyncTask mTask;
        final Data[] mData;
        
        private JWAsyncTaskResult(JWAsyncTask task, Data... data) {
            mTask = task;
            mData = data;
        }
    }
}
