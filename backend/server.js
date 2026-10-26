
const express = require("express");
const cors = require("cors");
const multer = require("multer");
const fs = require("fs");
const { execFile } = require("child_process");
const path = require("path");

const app = express();
const PORT = 3000;

const projectRoot = path.resolve(__dirname, "..");
const uploadsDirectory = path.join(projectRoot, "uploads");
const executablePath = path.join(projectRoot, "main.exe");

fs.mkdirSync(uploadsDirectory, { recursive: true });

// Multer storage configuration
const storage = multer.diskStorage({
    destination: (req, file, callback) => {
        callback(null, uploadsDirectory);
    },
    filename: (req, file, callback) => {
        const safeName = path.basename(file.originalname)
            .replace(/[^a-zA-Z0-9._-]/g, "_");

        callback(null, `${Date.now()}-${safeName}`);
    }
});

// Upload validation
const upload = multer({
    storage,
    limits: {
        fileSize: 5 * 1024 * 1024,
        files: 10
    },
    fileFilter: (req, file, callback) => {
        if (path.extname(file.originalname).toLowerCase() !== ".txt") {
            return callback(new Error("Only .txt files are allowed."));
        }

        callback(null, true);
    }
});

// Middleware
app.use(cors());
app.use(express.json());

// Test route
app.get("/", (req, res) => {
    res.json({
        message: "DocuSense backend is running!",
        status: "success"
    });
});

// Health-check route
app.get("/api/health", (req, res) => {
    res.json({
        status: "OK",
        service: "DocuSense API"
    });
});

// Get uploaded documents
app.get("/api/documents", async (req, res) => {
    try {
        const entries = await fs.promises.readdir(uploadsDirectory, {
            withFileTypes: true
        });

        const files = await Promise.all(
            entries
                .filter(entry =>
                    entry.isFile() &&
                    path.extname(entry.name).toLowerCase() === ".txt"
                )
                .map(async entry => ({
                    name: entry.name.replace(/^\d+-/, ""),
                    storedName: entry.name,
                    content: await fs.promises.readFile(
                        path.join(uploadsDirectory, entry.name),
                        "utf8"
                    )
                }))
        );

        return res.json({ files });
    } catch (error) {
        console.error("Could not load uploaded documents:", error);

        return res.status(500).json({
            error: "Uploaded documents could not be loaded."
        });
    }
});

// Search documents using the C++ engine
app.get("/api/search", (req, res) => {
    const keyword = req.query.keyword;

    if (typeof keyword !== "string" || !keyword.trim()) {
        return res.status(400).json({
            error: "Please provide a search keyword."
        });
    }

    execFile(
        executablePath,
        ["--search", keyword.trim()],
        {
            cwd: projectRoot,
            timeout: 10000,
            maxBuffer: 1024 * 1024
        },
        (error, stdout, stderr) => {
            if (error) {
                console.error("C++ search failed:", stderr || error.message);

                return res.status(500).json({
                    error: "The C++ search engine could not complete the request."
                });
            }

            try {
                const results = JSON.parse(stdout);

                return res.json({
                    keyword: keyword.trim(),
                    algorithm: "Boyer-Moore",
                    results
                });
            } catch (parseError) {
                console.error("Invalid C++ search output:", stdout);

                return res.status(500).json({
                    error: "The C++ engine returned an invalid response."
                });
            }
        }
    );
});

// Benchmark all three C++ search algorithms
app.get("/api/benchmark", (req, res) => {
    const keyword = req.query.keyword;

    if (typeof keyword !== "string" || !keyword.trim()) {
        return res.status(400).json({
            error: "Please provide a search keyword."
        });
    }

    execFile(
        executablePath,
        ["--benchmark", keyword.trim()],
        {
            cwd: projectRoot,
            timeout: 15000,
            maxBuffer: 1024 * 1024
        },
        (error, stdout, stderr) => {
            if (error) {
                console.error(
                    "C++ benchmark failed:",
                    stderr || error.message
                );

                return res.status(500).json({
                    error: "The C++ benchmark could not complete."
                });
            }

            try {
                const timings = JSON.parse(stdout);

                const validTimings = [
                    timings.linearSearch,
                    timings.kmpSearch,
                    timings.boyerMooreSearch
                ].every(value =>
                    typeof value === "number" &&
                    Number.isFinite(value) &&
                    value >= 0
                );

                if (!validTimings) {
                    throw new Error("Invalid timing values.");
                }

                return res.json({
                    keyword: keyword.trim(),
                    unit: "microseconds",
                    repetitions: 10000,
                    results: [
                        {
                            algorithm: "Naive Search",
                            time: timings.linearSearch
                        },
                        {
                            algorithm: "KMP Search",
                            time: timings.kmpSearch
                        },
                        {
                            algorithm: "Boyer-Moore",
                            time: timings.boyerMooreSearch
                        }
                    ]
                });
            } catch (parseError) {
                console.error("Invalid C++ benchmark output:", stdout);

                return res.status(500).json({
                    error: "The C++ engine returned invalid benchmark data."
                });
            }
        }
    );
});

// Delete an uploaded document
app.delete("/api/documents/:storedName", async (req, res) => {
    const { storedName } = req.params;

    if (
        path.basename(storedName) !== storedName ||
        path.extname(storedName).toLowerCase() !== ".txt"
    ) {
        return res.status(400).json({
            error: "Invalid document name."
        });
    }

    try {
        await fs.promises.unlink(
            path.join(uploadsDirectory, storedName)
        );

        return res.sendStatus(204);
    } catch (error) {
        if (error.code === "ENOENT") {
            return res.status(404).json({
                error: "Document was not found."
            });
        }

        console.error("Could not delete uploaded document:", error);

        return res.status(500).json({
            error: "Document could not be removed."
        });
    }
});

// Upload TXT documents
app.post("/api/upload", (req, res) => {
    upload.array("documents", 10)(req, res, (error) => {
        if (error) {
            return res.status(400).json({
                error: error.message || "Upload failed."
            });
        }

        if (!req.files || req.files.length === 0) {
            return res.status(400).json({
                error: "Please select at least one .txt file."
            });
        }

        const uploadedFiles = req.files.map(file => ({
            name: file.originalname,
            storedName: file.filename,
            size: file.size
        }));

        return res.status(201).json({
            message: "Documents uploaded successfully.",
            files: uploadedFiles
        });
    });
});

// Start server
app.listen(PORT, () => {
    console.log(`DocuSense backend running at http://localhost:${PORT}`);
});
