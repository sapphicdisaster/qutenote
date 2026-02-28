package com.qutenote.app;

import org.qtproject.qt.android.bindings.QtActivity;
import android.content.Intent;
import android.provider.MediaStore;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import java.io.File;
import android.os.Environment;
import androidx.core.content.FileProvider;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;

public class QuteNoteActivity extends QtActivity {
    private static QuteNoteActivity m_instance;
    private static final int REQUEST_IMAGE_CAPTURE = 1;
    private String currentPhotoPath;

    public QuteNoteActivity() {
        m_instance = this;
    }

    public static native void onImageReceived(String path);

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (m_instance == this) {
            m_instance = null;
        }
    }

    public static void startCameraIntent() {
        if (m_instance != null) {
            m_instance.dispatchTakePictureIntent();
        }
    }

    private void dispatchTakePictureIntent() {
        Intent takePictureIntent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
        
        File photoFile = null;
        try {
            photoFile = createImageFile();
        } catch (IOException ex) {
            Log.e("QuteNote", "Error creating image file", ex);
            return;
        }

        // Continue only if the File was successfully created
        if (photoFile != null) {
            Uri photoURI = null;
            try {
                photoURI = FileProvider.getUriForFile(this,
                        "com.qutenote.app.fileprovider",
                        photoFile);
            } catch (IllegalArgumentException e) {
                Log.e("QuteNote", "FileProvider error", e);
                return;
            }

            takePictureIntent.putExtra(MediaStore.EXTRA_OUTPUT, photoURI);
            
            // Grant URI permissions to the camera app
            takePictureIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
            
            try {
                startActivityForResult(takePictureIntent, REQUEST_IMAGE_CAPTURE);
            } catch (Exception e) {
                // Catch ActivityNotFoundException or SecurityException
                Log.e("QuteNote", "Failed to start camera activity", e);
            }
        }
    }

    private File createImageFile() throws IOException {
        String timeStamp = new SimpleDateFormat("yyyyMMdd_HHmmss").format(new Date());
        String imageFileName = "JPEG_" + timeStamp + "_";
        File storageDir = getExternalFilesDir(Environment.DIRECTORY_PICTURES);
        File image = File.createTempFile(
            imageFileName,  /* prefix */
            ".jpg",         /* suffix */
            storageDir      /* directory */
        );
        currentPhotoPath = image.getAbsolutePath();
        return image;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_IMAGE_CAPTURE && resultCode == RESULT_OK) {
            onImageReceived(currentPhotoPath);
        } else {
            super.onActivityResult(requestCode, resultCode, data);
        }
    }
}
