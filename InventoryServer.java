
import com.sun.net.httpserver.Headers;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import com.sun.net.httpserver.HttpServer;
import java.io.*;
import java.net.InetSocketAddress;
import java.net.URI;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.*;
import java.util.concurrent.Executors;

public class InventoryServer {

    private static final int PORT = 8000;
    private static final Path CSV = Path.of("inventory.csv");

    public static void main(String[] args) throws Exception {
        // try to launch C++ executable if present
        launchCppIfPresent();

        HttpServer server = HttpServer.create(new InetSocketAddress(PORT), 0);
        server.createContext("/", new StaticHandler("web"));
        server.createContext("/api/list", new ListHandler());
        server.createContext("/api/add", new AddHandler());
        server.createContext("/api/remove", new RemoveHandler());
        server.setExecutor(Executors.newFixedThreadPool(4));
        server.start();
        System.out.println("✓ Inventory Server started at http://localhost:" + PORT);
    }

    @SuppressWarnings("unused")
    private static void launchCppIfPresent() {
        Path exe = Path.of("main.exe");
        if (!Files.exists(exe)) {
            exe = Path.of("main");
        }
        if (Files.exists(exe)) {
            try {
                ProcessBuilder pb = new ProcessBuilder(exe.toString());
                pb.redirectOutput(ProcessBuilder.Redirect.appendTo(Path.of("cpp.out.log").toFile()));
                pb.redirectError(ProcessBuilder.Redirect.appendTo(Path.of("cpp.err.log").toFile()));
                pb.start();  // Process runs in background
                System.out.println("✓ C++ process launched: " + exe);
            } catch (IOException e) {
                System.err.println("⚠ Failed to launch C++ executable: " + e.getMessage());
            }
        } else {
            System.out.println("ℹ No C++ executable found (main.exe or main). Skipping launch.");
        }
    }

    static List<Map<String, String>> readAll() throws IOException {
        List<Map<String, String>> out = new ArrayList<>();
        if (!Files.exists(CSV)) {
            return out;
        }
        List<String> lines = Files.readAllLines(CSV);
        for (String line : lines) {
            // expect: id,"name",base,tax,discount
            String[] parts = parseLine(line);
            if (parts == null) {
                continue;
            }
            Map<String, String> m = new HashMap<>();
            m.put("id", parts[0]);
            m.put("name", parts[1]);
            m.put("base", parts[2]);
            m.put("tax", parts[3]);
            m.put("discount", parts[4]);
            out.add(m);
        }
        return out;
    }

    static String[] parseLine(String line) {
        // basic: split id, "name", rest
        try {
            int firstComma = line.indexOf(',');
            if (firstComma < 0) {
                return null;
            }
            String id = line.substring(0, firstComma);
            int quote1 = line.indexOf('"', firstComma);
            int quote2 = line.indexOf('"', quote1 + 1);
            if (quote1 < 0 || quote2 < 0) {
                return null;
            }
            String name = line.substring(quote1 + 1, quote2);
            String rest = line.substring(quote2 + 2);
            String[] nums = rest.split(",");
            if (nums.length < 3) {
                return null;
            }
            return new String[]{id.trim(), name, nums[0].trim(), nums[1].trim(), nums[2].trim()};
        } catch (Exception e) {
            return null;
        }
    }

    static void writeAll(List<Map<String, String>> data) throws IOException {
        try (BufferedWriter w = Files.newBufferedWriter(CSV, StandardCharsets.UTF_8)) {
            for (Map<String, String> m : data) {
                String line = String.format("%s,\"%s\",%s,%s,%s",
                        m.get("id"), m.get("name"), m.get("base"), m.get("tax"), m.get("discount"));
                w.write(line);
                w.newLine();
            }
        }
    }

    static void sendJson(HttpExchange ex, int code, String body) throws IOException {
        byte[] bytes = body.getBytes(StandardCharsets.UTF_8);
        Headers h = ex.getResponseHeaders();
        h.set("Content-Type", "application/json; charset=utf-8");
        ex.sendResponseHeaders(code, bytes.length);
        try (OutputStream os = ex.getResponseBody()) {
            os.write(bytes);
        }
    }

    static class ListHandler implements HttpHandler {

        @Override
        public void handle(HttpExchange ex) throws IOException {
            List<Map<String, String>> all = readAll();
            StringBuilder sb = new StringBuilder();
            sb.append("[");
            boolean first = true;
            for (var m : all) {
                if (!first) {
                    sb.append(',');

                }
                first = false;
                sb.append("{");
                sb.append("\"id\":").append(m.get("id")).append(',');
                sb.append("\"name\":\"").append(escapeJson(m.get("name"))).append("\",");
                sb.append("\"base\":").append(m.get("base")).append(',');
                sb.append("\"tax\":").append(m.get("tax")).append(',');
                sb.append("\"discount\":").append(m.get("discount"));
                sb.append("}");
            }
            sb.append("]");
            sendJson(ex, 200, sb.toString());
        }
    }

    static class AddHandler implements HttpHandler {

        @Override
        public void handle(HttpExchange ex) throws IOException {
            if (!"POST".equalsIgnoreCase(ex.getRequestMethod())) {
                sendJson(ex, 405, "{\"error\":\"method\"}");
                return;
            }
            String body = new String(ex.getRequestBody().readAllBytes(), StandardCharsets.UTF_8);
            Map<String, String> payload = parseJson(body);
            if (payload == null || !payload.containsKey("id")) {
                sendJson(ex, 400, "{\"error\":\"bad json\"}");
                return;
            }
            List<Map<String, String>> all = readAll();
            for (var m : all) {
                if (m.get("id").equals(payload.get("id"))) {
                    sendJson(ex, 409, "{\"error\":\"exists\"}");
                    return;
                }
            }
            Map<String, String> item = new HashMap<>();
            item.put("id", payload.get("id"));
            item.put("name", payload.getOrDefault("name", ""));
            item.put("base", payload.getOrDefault("base", "0"));
            item.put("tax", payload.getOrDefault("tax", "0"));
            item.put("discount", payload.getOrDefault("discount", "0"));
            all.add(item);
            writeAll(all);
            sendJson(ex, 201, "{\"ok\":true}");
        }
    }

    static class RemoveHandler implements HttpHandler {

        @Override
        public void handle(HttpExchange ex) throws IOException {
            if (!"POST".equalsIgnoreCase(ex.getRequestMethod())) {
                sendJson(ex, 405, "{\"error\":\"method\"}");
                return;
            }
            String body = new String(ex.getRequestBody().readAllBytes(), StandardCharsets.UTF_8);
            Map<String, String> payload = parseJson(body);
            if (payload == null || !payload.containsKey("id")) {
                sendJson(ex, 400, "{\"error\":\"bad json\"}");
                return;
            }
            List<Map<String, String>> all = readAll();
            String id = payload.get("id");
            boolean removed = all.removeIf(m -> m.get("id").equals(id));
            if (removed) {
                writeAll(all);
            }
            sendJson(ex, 200, "{\"removed\":" + removed + "}");
        }
    }

    static Map<String, String> parseJson(String s) {
        // very small and forgiving parser for flat JSON with string/number values
        try {
            Map<String, String> m = new HashMap<>();
            s = s.trim();
            if (s.startsWith("{")) {
                s = s.substring(1);

            }
            if (s.endsWith("}")) {
                s = s.substring(0, s.length() - 1);
            }
            String[] parts = s.split(",");
            for (String part : parts) {
                String[] kv = part.split(":", 2);
                if (kv.length != 2) {
                    continue;
                }
                String k = kv[0].trim();
                if (k.startsWith("\"") && k.endsWith("\"")) {
                    k = k.substring(1, k.length() - 1);
                }
                String v = kv[1].trim();
                if (v.startsWith("\"") && v.endsWith("\"")) {
                    v = v.substring(1, v.length() - 1);
                }
                m.put(k, v);
            }
            return m;
        } catch (Exception e) {
            return null;
        }
    }

    static String escapeJson(String s) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    static class StaticHandler implements HttpHandler {

        private final Path root;

        StaticHandler(String rootDir) {
            root = Path.of(rootDir);
        }

        @Override
        public void handle(HttpExchange ex) throws IOException {
            URI uri = ex.getRequestURI();
            Path file = root.resolve(uri.getPath().substring(1)).normalize();
            if (!file.startsWith(root)) {
                ex.sendResponseHeaders(403, -1);
                return;
            }
            if (Files.isDirectory(file) || uri.getPath().equals("/")) {
                file = root.resolve("index.html");
            }
            if (!Files.exists(file)) {
                ex.sendResponseHeaders(404, -1);
                return;
            }
            byte[] bytes = Files.readAllBytes(file);
            String ct = Files.probeContentType(file);
            if (ct == null) {
                ct = "application/octet-stream";
            }
            ex.getResponseHeaders().set("Content-Type", ct);
            ex.sendResponseHeaders(200, bytes.length);
            try (OutputStream os = ex.getResponseBody()) {
                os.write(bytes);
            }
        }
    }
}
