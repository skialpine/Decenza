package io.github.kulitorum.decenza_de1;

import android.os.Bundle;

import org.qtproject.qt.android.bindings.QtActivity;

public class DecenzaActivity extends QtActivity {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        StorageHelper.init(this);
    }
}
