
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
            timeout: 30000,
            maxBuffer: 5 * 1024 * 1024
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
                const benchmarkData = JSON.parse(stdout);

                if (!Array.isArray(benchmarkData)) {
                    throw new Error(
                        "Expected an array of benchmark test cases."
                    );
                }

                const sizeMap = {
                    1000: "small",
                    10000: "medium",
                    100000: "large"
                };

                const algorithmMap = {
                    "Iterative Search": {
                        totalKey: "linearSearch",
                        averageKey: "linearAverage"
                    },
                    "KMP": {
                        totalKey: "kmpSearch",
                        averageKey: "kmpAverage"
                    },
                    "Boyer-Moore (bad-character)": {
                        totalKey: "boyerMooreSearch",
                        averageKey: "boyerMooreAverage"
                    }
                };

                const groupedResults = {};

                for (const test of benchmarkData) {
                    const size = sizeMap[test.text_size];

                    if (!size || !Array.isArray(test.results)) {
                        throw new Error(
                            "Unexpected benchmark test-case format."
                        );
                    }

                    if (!groupedResults[size]) {
                        groupedResults[size] = {
                            size,
                            characters: test.text_size,
                            repetitions: 0,
                            linearSearch: 0,
                            kmpSearch: 0,
                            boyerMooreSearch: 0,
                            linearAverage: 0,
                            kmpAverage: 0,
                            boyerMooreAverage: 0,
                            testCases: 0
                        };
                    }

                    const group = groupedResults[size];
                    group.testCases++;

                    for (const result of test.results) {
                        const mapping = algorithmMap[result.algorithm];

                        if (!mapping) {
                            throw new Error(
                                `Unknown algorithm: ${result.algorithm}`
                            );
                        }

                        const total =
                            Number(result.search_total_microseconds);

                        const average =
                            Number(result.average_search_microseconds);

                        if (
                            !Number.isFinite(total) ||
                            !Number.isFinite(average) ||
                            total < 0 ||
                            average < 0
                        ) {
                            throw new Error(
                                "Invalid benchmark timing values."
                            );
                        }

                        group[mapping.totalKey] += total;
                        group[mapping.averageKey] += average;
                    }

                    group.repetitions += Number(test.repetitions) || 0;
                }

                const results = ["small", "medium", "large"].map(size => {
                    const group = groupedResults[size];

                    if (!group || group.testCases === 0) {
                        throw new Error(
                            `Missing ${size} benchmark results.`
                        );
                    }

                    // Average the timings across the test patterns.
                    for (const key of [
                        "linearAverage",
                        "kmpAverage",
                        "boyerMooreAverage"
                    ]) {
                        group[key] /= group.testCases;
                    }

                    delete group.testCases;

                    return group;
                });

                return res.json({
                    keyword: keyword.trim(),
                    unit: "microseconds",
                    results
                });

            } catch (parseError) {
                console.error(
                    "Invalid C++ benchmark output:",
                    stdout,
                    parseError.message
                );

                return res.status(500).json({
                    error:
                        "The C++ engine returned invalid benchmark data."
                });
            }
        }
    );
});

/* Compare two documents using the C++ LCS algorithm */
app.post("/api/similarity", (req, res) => {
    const { document1, document2 } = req.body;

    if (
        typeof document1 !== "string" ||
        typeof document2 !== "string" ||
        !document1.trim() ||
        !document2.trim()
    ) {
        return res.status(400).json({
            error: "Please select two documents to compare."
        });
    }

    // Resolve only known sample documents or uploaded files.
    function resolveDocument(name) {
        const sampleFiles = {
            "sample1.txt": "documents/sample1.txt",
            "sample2.txt": "documents/sample2.txt",
            "sample3.txt": "documents/sample3.txt"
        };

        if (sampleFiles[name]) {
            return sampleFiles[name];
        }

        // Uploaded files use their storedName, not their display name.
        if (
            path.basename(name) === name &&
            path.extname(name).toLowerCase() === ".txt"
        ) {
            const uploadedPath = path.join(uploadsDirectory, name);

            if (fs.existsSync(uploadedPath)) {
                return path.join("uploads", name);
            }
        }

        return null;
    }

    const file1 = resolveDocument(document1);
    const file2 = resolveDocument(document2);

    if (!file1 || !file2) {
        return res.status(404).json({
            error: "One or both documents could not be found."
        });
    }

    if (file1 === file2) {
        return res.status(400).json({
            error: "Please select two different documents."
        });
    }

    execFile(
        executablePath,
        ["--similarity", file1, file2],
        {
            cwd: projectRoot,
            timeout: 15000,
            maxBuffer: 1024 * 1024
        },
        (error, stdout, stderr) => {
            if (error) {
                console.error(
                    "C++ similarity calculation failed:",
                    stderr || error.message
                );

                return res.status(500).json({
                    error: "Could not calculate document similarity."
                });
            }

            try {
                const scores = JSON.parse(stdout);

                return res.json({
                    document1,
                    document2,
                    lcsSimilarity: scores.lcsSimilarity
                });
            } catch (parseError) {
                console.error("Invalid similarity output:", stdout);

                return res.status(500).json({
                    error: "The C++ engine returned invalid similarity data."
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
