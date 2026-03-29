Overview

This workspace contains three components:

- `main.c++` — feature-rich CLI inventory & pricing program (reads/writes `inventory.csv`).
- `src/InventoryServer.java` — lightweight Java HTTP server that serves the frontend and provides a simple API backed by `inventory.csv`. It also attempts to launch the C++ executable (`main.exe` or `main`) if present.
- `web/index.html` & `web/app.js` — frontend UI.

Quick start (Windows PowerShell):

```powershell
# run the provided script which compiles and launches components
.\run.ps1
```

Notes

- Java requires a JDK (`javac` & `java`) in PATH.
- C++ build uses `g++` if available; if not present, place a compiled `main.exe` in the folder.
- The Java server and the C++ process both use `inventory.csv` for storage; avoid concurrent edits for production use.

GitHub Pages Deployment

- This project now supports static client-side mode using `web/main.html` + `web/main.js`.
- No backend is required; inventory data is stored in browser localStorage under key `inventory-data-v1`.
- To deploy: push repository to GitHub and enable Pages from the `gh-pages` branch or `/docs` folder mapped to `web/` (copy `web/*` to your published folder).
- Access via `https://<username>.github.io/<repo>/main.html`.

Next steps

- Add robust JSON parsing/validation (use a library like Jackson)
- Replace CSV with a tiny embedded DB (SQLite) for safer concurrent access
- Add authentication and auditing for multi-user scenarios
