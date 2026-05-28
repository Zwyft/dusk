package com.twilitrealm.dusk;

import android.Manifest;
import android.app.ActionBar;
import android.content.ClipData;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.TextView;
import org.libsdl.app.SDLActivity;

import java.io.BufferedReader;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class DuskActivity extends SDLActivity {
    private static final String TAG = "DuskActivity";
    private static final String CRASH_LOG_FILE = "dusk_crash.log";
    private static final int LOG_SERVER_PORT = 8080;
    private ExecutorService logExecutor;
    private ServerSocket logServerSocket;
    private static final int STORAGE_PERMISSION_REQUEST_CODE = 100;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // IMMEDIATE crash log to /sdcard/dusk_startup.txt (no permissions needed)
        try {
            java.io.FileWriter fw = new java.io.FileWriter("/sdcard/dusk_startup.txt", true);
            fw.write("=== Dusk onCreate() called at " + new java.util.Date() + " ===\n");
            fw.write("Step 1: Writing startup log\n");
            fw.close();
        } catch (Exception e) { /* ignore */ }
        
        // Start HTTP log server FIRST before anything else
        startLogServer();
        
        // Install global crash handler BEFORE super.onCreate()
        Thread.setDefaultUncaughtExceptionHandler(new Thread.UncaughtExceptionHandler() {
            @Override
            public void uncaughtException(Thread thread, Throwable ex) {
                logCrash(thread, ex);
                // Re-throw to default handler
                Thread.getDefaultUncaughtExceptionHandler().uncaughtException(thread, ex);
            }
        });
        
        try {
            super.onCreate(savedInstanceState);
            hideSystemBars();
            requestStoragePermissions();
        } catch (Exception e) {
            try {
                java.io.FileWriter fw = new java.io.FileWriter("/sdcard/dusk_crash.txt", true);
                fw.write("=== EXCEPTION in onCreate() ===\n");
                fw.write("Time: " + new java.util.Date() + "\n");
                fw.write("Error: " + e.toString() + "\n");
                java.io.PrintWriter pw = new java.io.PrintWriter(fw);
                e.printStackTrace(pw);
                pw.close();
            } catch (Exception e2) { /* ignore */ }
        }
    }
    
    private void startLogServer() {
        logExecutor = Executors.newFixedThreadPool(3);
        
        // Start continuous logcat saving to file
        logExecutor.execute(new Runnable() {
            @Override
            public void run() {
                saveLogcatToFile();
            }
        });
        
        logExecutor.execute(new Runnable() {
            @Override
            public void run() {
                try {
                    logServerSocket = new ServerSocket(LOG_SERVER_PORT);
                    Log.i(TAG, "=== LOG SERVER STARTED ===");
                    Log.i(TAG, "Connect to: http://" + getDeviceIP() + ":" + LOG_SERVER_PORT);
                    Log.i(TAG, "==========================");
                    
                    while (!logServerSocket.isClosed()) {
                        try {
                            Socket client = logServerSocket.accept();
                            logExecutor.execute(new LogRequestHandler(client, DuskActivity.this));
                        } catch (IOException e) {
                            if (!logServerSocket.isClosed()) {
                                Log.e(TAG, "Error accepting connection", e);
                            }
                        }
                    }
                } catch (IOException e) {
                    Log.e(TAG, "Failed to start log server on port " + LOG_SERVER_PORT, e);
                }
            }
        });
    }
    
    private void saveLogcatToFile() {
        try {
            File logDir = getExternalFilesDir(null);
            if (logDir == null) logDir = getFilesDir();
            File logFile = new File(logDir, "dusk_live_log.txt");
            
            // Clear old log and start fresh
            Runtime.getRuntime().exec("logcat -c");
            Log.i(TAG, "Started saving logcat to: " + logFile.getAbsolutePath());
            
            // Run logcat -f to continuously save to file
            Process logcat = Runtime.getRuntime().exec("logcat -v time -f " + logFile.getAbsolutePath());
            logcat.waitFor();
        } catch (IOException | InterruptedException e) {
            Log.e(TAG, "Error saving logcat to file", e);
        }
    }
    
    private String getDeviceIP() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && !(inetAddress instanceof Inet6Address)) {
                        String ip = inetAddress.getHostAddress();
                        if (ip != null && ip.contains(".")) {
                            return ip;
                        }
                    }
                }
            }
        } catch (SocketException e) {
            Log.e(TAG, "Error getting device IP", e);
        }
        return "127.0.0.1";
    }
    
    private File getLiveLogFile() {
        File logDir = getExternalFilesDir(null);
        if (logDir == null) logDir = getFilesDir();
        return new File(logDir, "dusk_live_log.txt");
    }
    
    private static class LogRequestHandler implements Runnable {
        private final Socket client;
        private final DuskActivity activity;
        
        LogRequestHandler(Socket client, DuskActivity activity) {
            this.client = client;
            this.activity = activity;
        }
        
        @Override
        public void run() {
            try {
                BufferedReader in = new BufferedReader(new InputStreamReader(client.getInputStream()));
                String requestLine = in.readLine();
                
                if (requestLine == null) {
                    client.close();
                    return;
                }
                
                String path = requestLine.split(" ")[1];
                OutputStream out = client.getOutputStream();
                
                if (path.equals("/") || path.equals("/index.html")) {
                    sendHtmlPage(out);
                } else if (path.equals("/download")) {
                    sendCrashLog(out);
                } else if (path.equals("/raw")) {
                    sendRawLogs(out);
                } else {
                    send404(out);
                }
                
                out.flush();
                client.close();
            } catch (IOException e) {
                Log.e(TAG, "Error handling log request", e);
            }
        }
        
        private void sendHtmlPage(OutputStream out) throws IOException {
            String ip = activity.getDeviceIP();
            String html = "<!DOCTYPE html>" +
                "<html><head><title>Dusk Debug Log</title>" +
                "<meta http-equiv='refresh' content='2'>" +
                "<style>body{font-family:monospace;background:#000;color:#0f0;padding:20px;" +
                "white-space:pre-wrap;word-wrap:break-word;}" +
                "h1{color:#0ff;}a{color:#ff0;}</style></head><body>" +
                "<h1>🎮 Dusk Debug Log</h1>" +
                "<p><b>Device IP:</b> " + ip + "<br>" +
                "<b>Connect at:</b> http://" + ip + ":8080</p>" +
                "<hr><p><a href='/download'>📥 Download Full Live Log</a> | " +
                "<a href='/raw'>📄 Raw Log Text</a></p><hr>" +
                "<b>=== Recent Logcat (Last 500 Lines) ===</b><br><br>";
            
            out.write(("HTTP/1.1 200 OK\r\n" +
                "Content-Type: text/html\r\n" +
                "Connection: close\r\n" +
                "\r\n" + html).getBytes());
            
            // Read from saved log file
            File logFile = activity.getLiveLogFile();
            if (logFile.exists()) {
                // Read last 500 lines
                BufferedReader reader = new BufferedReader(new java.io.FileReader(logFile));
                java.util.Deque<String> lastLines = new java.util.ArrayDeque<>(500);
                String line;
                while ((line = reader.readLine()) != null) {
                    if (lastLines.size() >= 500) {
                        lastLines.poll();
                    }
                    lastLines.add(line);
                }
                reader.close();
                
                for (String l : lastLines) {
                    out.write((l + "<br>").getBytes());
                }
            } else {
                out.write("Log file not yet created. Please wait...".getBytes());
            }
            
            out.write("</body></html>".getBytes());
        }
        
        private void sendCrashLog(OutputStream out) throws IOException {
            File logFile = activity.getLiveLogFile();
            
            out.write(("HTTP/1.1 200 OK\r\n" +
                "Content-Type: text/plain\r\n" +
                "Content-Disposition: attachment; filename=\"dusk_live_log.txt\"\r\n" +
                "Connection: close\r\n" +
                "\r\n").getBytes());
            
            if (logFile.exists()) {
                InputStream in = new java.io.FileInputStream(logFile);
                byte[] buf = new byte[8192];
                int len;
                while ((len = in.read(buf)) > 0) {
                    out.write(buf, 0, len);
                }
                in.close();
            } else {
                out.write("Log file not yet created. Please wait...".getBytes());
            }
        }
        
        private void sendRawLogs(OutputStream out) throws IOException {
            out.write(("HTTP/1.1 200 OK\r\n" +
                "Content-Type: text/plain\r\n" +
                "Connection: close\r\n" +
                "\r\n").getBytes());
            
            File logFile = activity.getLiveLogFile();
            if (logFile.exists()) {
                InputStream in = new java.io.FileInputStream(logFile);
                byte[] buf = new byte[8192];
                int len;
                while ((len = in.read(buf)) > 0) {
                    out.write(buf, 0, len);
                }
                in.close();
            } else {
                out.write("Log file not yet created. Please wait...".getBytes());
            }
        }
        
        private void send404(OutputStream out) throws IOException {
            out.write(("HTTP/1.1 404 Not Found\r\n" +
                "Content-Type: text/plain\r\n" +
                "Connection: close\r\n" +
                "\r\n404 Not Found").getBytes());
        }
    }
    
    private void logCrash(Thread thread, Throwable ex) {
        try {
            // Write to file in app's external files dir
            File logDir = getExternalFilesDir(null);
            if (logDir == null) {
                logDir = getFilesDir();
            }
            File logFile = new File(logDir, CRASH_LOG_FILE);
            
            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            pw.println("=== Dusk Crash Report ===");
            pw.println("Time: " + new java.util.Date());
            pw.println("Thread: " + thread.getName());
            pw.println("Android: " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")");
            pw.println("Device: " + Build.MANUFACTURER + " " + Build.MODEL);
            pw.println("ABI: " + Build.SUPPORTED_ABIS[0]);
            pw.println();
            ex.printStackTrace(pw);
            pw.println();
            pw.println("=== END ===");
            
            try (FileOutputStream fos = new FileOutputStream(logFile, true)) {
                fos.write(sw.toString().getBytes());
            }
            
            Log.e(TAG, "Crash logged to: " + logFile.getAbsolutePath());
        } catch (Exception e) {
            Log.e(TAG, "Failed to log crash", e);
        }
    }

    private void requestStoragePermissions() {
        // Android 11+ (API 30+) MANAGE_EXTERNAL_STORAGE
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            if (!Environment.isExternalStorageManager()) {
                Log.i(TAG, "Requesting MANAGE_EXTERNAL_STORAGE permission");
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION);
                Uri uri = Uri.fromParts("package", getPackageName(), null);
                intent.setData(uri);
                startActivity(intent);
                return;
            }
        }

        // Android 6.0+ (API 23+) READ/WRITE_EXTERNAL_STORAGE
        List<String> permissionsNeeded = new ArrayList<>();
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.READ_EXTERNAL_STORAGE);
        }
        if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.WRITE_EXTERNAL_STORAGE);
        }

        if (!permissionsNeeded.isEmpty()) {
            Log.i(TAG, "Requesting storage permissions: " + permissionsNeeded);
            requestPermissions(
                    permissionsNeeded.toArray(new String[0]),
                    STORAGE_PERMISSION_REQUEST_CODE);
        } else {
            Log.i(TAG, "All storage permissions already granted");
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions,
            int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == STORAGE_PERMISSION_REQUEST_CODE) {
            boolean allGranted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }
            if (allGranted) {
                Log.i(TAG, "Storage permissions granted");
            } else {
                Log.w(TAG, "Storage permissions denied - some features may not work");
            }
        }
    }

    private static String[] splitArgs(String raw) {
        List<String> out = new ArrayList<>();
        StringBuilder current = new StringBuilder();
        boolean inSingle = false;
        boolean inDouble = false;
        boolean escaped = false;

        for (int i = 0; i < raw.length(); ++i) {
            char c = raw.charAt(i);
            if (escaped) {
                current.append(c);
                escaped = false;
                continue;
            }
            if (c == '\\' && !inSingle) {
                escaped = true;
                continue;
            }
            if (c == '"' && !inSingle) {
                inDouble = !inDouble;
                continue;
            }
            if (c == '\'' && !inDouble) {
                inSingle = !inSingle;
                continue;
            }
            if (!inSingle && !inDouble && Character.isWhitespace(c)) {
                if (current.length() > 0) {
                    out.add(current.toString());
                    current.setLength(0);
                }
                continue;
            }
            current.append(c);
        }

        if (escaped) {
            current.append('\\');
        }
        if (current.length() > 0) {
            out.add(current.toString());
        }
        return out.toArray(new String[0]);
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        hideSystemBars();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemBars();
        }
    }

    private void hideSystemBars() {
        Window window = getWindow();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController ctrl = window.getDecorView().getWindowInsetsController();
            if (ctrl != null) {
                ctrl.setSystemBarsBehavior(
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                ctrl.hide(WindowInsets.Type.systemBars());
            }
        } else {
            View decorView = window.getDecorView();
            int uiOptions = View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE;
            decorView.setSystemUiVisibility(uiOptions);
            ActionBar actionBar = getActionBar();
            if (actionBar != null) {
                actionBar.hide();
            }
        }
    }

    @Override
    protected String[] getLibraries() {
        // SDL3 is statically linked into libmain.so in this build.
        return new String[] {
            "main"
        };
    }

    @Override
    protected String[] getArguments() {
        Intent intent = getIntent();
        if (intent != null) {
            String[] argv = intent.getStringArrayExtra("dusk_argv");
            if (argv != null && argv.length > 0) {
                return argv;
            }

            String rawArgs = intent.getStringExtra("dusk_args");
            if (rawArgs != null) {
                String trimmed = rawArgs.trim();
                if (!trimmed.isEmpty()) {
                    return splitArgs(trimmed);
                }
            }
        }
        return new String[0];
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode == RESULT_OK) {
            persistUriPermissions(data);
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    private void persistUriPermissions(Intent data) {
        if (data == null) {
            return;
        }

        int permissionFlags =
            data.getFlags() & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);
        if (permissionFlags == 0) {
            return;
        }

        Uri uri = data.getData();
        if (uri != null) {
            persistUriPermission(uri, permissionFlags);
        }

        ClipData clipData = data.getClipData();
        if (clipData == null) {
            return;
        }
        for (int i = 0; i < clipData.getItemCount(); ++i) {
            Uri itemUri = clipData.getItemAt(i).getUri();
            if (itemUri != null) {
                persistUriPermission(itemUri, permissionFlags);
            }
        }
    }

    private void persistUriPermission(Uri uri, int permissionFlags) {
        if ((permissionFlags & Intent.FLAG_GRANT_READ_URI_PERMISSION) != 0) {
            persistUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION, "read");
        }
        if ((permissionFlags & Intent.FLAG_GRANT_WRITE_URI_PERMISSION) != 0) {
            persistUriPermission(uri, Intent.FLAG_GRANT_WRITE_URI_PERMISSION, "write");
        }
    }

    private void persistUriPermission(Uri uri, int permissionFlag, String permissionName) {
        try {
            getContentResolver().takePersistableUriPermission(uri, permissionFlag);
        } catch (SecurityException | IllegalArgumentException e) {
            Log.w(TAG, "Unable to persist " + permissionName + " URI permission for " + uri, e);
        }
    }

    public String getDisplayNameForUri(String uriString) {
        if (uriString == null || uriString.isEmpty()) {
            return "";
        }

        Uri uri = Uri.parse(uriString);
        if ("content".equals(uri.getScheme())) {
            try (Cursor cursor = getContentResolver().query(
                uri, new String[] { OpenableColumns.DISPLAY_NAME }, null, null, null))
            {
                if (cursor != null && cursor.moveToFirst()) {
                    int displayNameColumn = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                    if (displayNameColumn >= 0) {
                        String displayName = cursor.getString(displayNameColumn);
                        if (displayName != null && !displayName.isEmpty()) {
                            return displayName;
                        }
                    }
                }
            } catch (SecurityException | IllegalArgumentException e) {
                Log.w(TAG, "Unable to query display name for " + uri, e);
            }
        } else if ("file".equals(uri.getScheme())) {
            String path = uri.getPath();
            if (path != null && !path.isEmpty()) {
                String name = new File(path).getName();
                if (!name.isEmpty()) {
                    return name;
                }
            }
        }

        String lastSegment = uri.getLastPathSegment();
        return lastSegment != null ? lastSegment : "";
    }

    public String copyContentUriToTempFile(String uriString) {
        if (uriString == null || uriString.isEmpty()) {
            return "";
        }

        Uri uri = Uri.parse(uriString);
        if (!"content".equals(uri.getScheme())) {
            return uriString;
        }

        String displayName = getDisplayNameForUri(uriString);
        if (displayName.isEmpty()) {
            displayName = "disc.iso";
        }

        File cacheDir = getCacheDir();
        File tempFile = new File(cacheDir, displayName);

        try (InputStream in = getContentResolver().openInputStream(uri);
             FileOutputStream out = new FileOutputStream(tempFile)) {
            if (in == null) {
                Log.e(TAG, "Unable to open input stream for " + uri);
                return "";
            }
            byte[] buf = new byte[8192];
            int len;
            while ((len = in.read(buf)) > 0) {
                out.write(buf, 0, len);
            }
            out.flush();
            return tempFile.getAbsolutePath();
        } catch (IOException | SecurityException | IllegalArgumentException e) {
            Log.e(TAG, "Failed to copy content URI to temp file: " + uri, e);
            return "";
        }
    }
}
