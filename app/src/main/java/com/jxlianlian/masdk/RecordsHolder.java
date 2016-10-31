package com.jxlianlian.masdk;

import java.util.LinkedList;
import java.util.List;

/**
 * Created by zhourui on 16/10/27.
 */

public class RecordsHolder {
  private List<Record> records = new LinkedList<>();

  public void addItem(int beginTime, int endTime, String playbackUrl, String downloadUrl) {
    Record record = new Record(beginTime, endTime, playbackUrl, downloadUrl);
    records.add(record);
  }

  public int size() {
    return records.size();
  }

  public List<Record> list() {
    return records;
  }
}
