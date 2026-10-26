
let documents = [
    {
        name: "sample1.txt",
        content: "Merge sort is a sorting algorithm that uses divide and conquer."
    },
    {
        name: "sample2.txt",
        content: "Quick sort and merge sort are important sorting algorithms."
    },
    {
        name: "sample3.txt",
        content: "Operating systems manage memory and processes."
    }
];

// Convert text to lowercase and extract unique words.
function getUniqueWords(text) {
    const words = text.toLowerCase().match(/[a-z0-9]+/g) || [];
    return new Set(words);
}

// Calculate Jaccard similarity as a percentage.
function calculateJaccardSimilarity(textA, textB) {
    const wordsA = getUniqueWords(textA);
    const wordsB = getUniqueWords(textB);

    const union = new Set([...wordsA, ...wordsB]);

    if (union.size === 0) {
        return 0;
    }

    let intersection = 0;

    for (const word of wordsA) {
        if (wordsB.has(word)) {
            intersection++;
        }
    }

    return (intersection / union.size) * 100;
}

// Escape text before displaying it in HTML.
function escapeHTML(text) {
    return String(text).replace(/[&<>"']/g, character => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[character]);
}

// Search documents using the C++ engine through Node.js.
const searchForm = document.getElementById("searchForm");
const searchInput = document.getElementById("searchInput");
const searchResults = document.getElementById("searchResults");
const resultCount = document.getElementById("resultCount");

async function displaySearchResults(keyword) {
    const normalizedKeyword = keyword.trim();

    if (!normalizedKeyword) {
        return;
    }

    resultCount.textContent = "Searching...";
    searchResults.innerHTML = `
        <div class="empty-state">
            <p>Running the C++ search engine...</p>
        </div>
    `;

    try {
        const response = await fetch(
            `http://localhost:3000/api/search?keyword=${encodeURIComponent(normalizedKeyword)}`
        );

        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.error || "Search request failed.");
        }

        const matches = data.results || [];

        resultCount.textContent =
            `${matches.length} document${matches.length === 1 ? "" : "s"} found`;

        if (matches.length === 0) {
            searchResults.innerHTML = `
                <div class="empty-state">
                    <span>⌕</span>
                    <p>No documents matched "${escapeHTML(normalizedKeyword)}".</p>
                </div>
            `;
            return;
        }

        searchResults.innerHTML = matches.map(filename => {
            const document = documents.find(item =>
                filename === `documents/${item.name}` ||
                filename === `uploads/${item.storedName}` ||
                filename.endsWith("/" + item.name) ||
                filename.endsWith("\\" + item.name)
            );

            const displayName = document
                ? document.name
                : filename.split(/[\\/]/).pop();

            const content = document
                ? document.content
                : "This document matched the keyword in the C++ engine.";

            return `
                <article class="result-card">
                    <div>
                        <h4>${escapeHTML(displayName)}</h4>
                        <p>${escapeHTML(content)}</p>
                    </div>
                    <span class="result-badge">C++ Match</span>
                </article>
            `;
        }).join("");

    } catch (error) {
        resultCount.textContent = "Search failed";
        searchResults.innerHTML = `
            <div class="empty-state">
                <p>${escapeHTML(error.message)}</p>
                <p>Make sure the Node.js server is running on port 3000.</p>
            </div>
        `;
    }
}

searchForm.addEventListener("submit", async event => {
    event.preventDefault();

    const keyword = searchInput.value.trim();

    if (keyword) {
        await displaySearchResults(keyword);
    }
});

// Quick-search keyword buttons.
document.querySelectorAll(".keyword-chip").forEach(button => {
    button.addEventListener("click", () => {
        const keyword = button.dataset.keyword;
        searchInput.value = keyword;
        displaySearchResults(keyword);
    });
});

// Calculate Jaccard similarity for every document pair.
function analyzeDocuments() {
    const similarityResults =
        document.getElementById("similarityResults");

    const pairs = [];

    for (let i = 0; i < documents.length; i++) {
        for (let j = i + 1; j < documents.length; j++) {
            const similarity = calculateJaccardSimilarity(
                documents[i].content,
                documents[j].content
            );

            pairs.push({
                documentA: documents[i].name,
                documentB: documents[j].name,
                similarity
            });
        }
    }

    pairs.sort((a, b) => b.similarity - a.similarity);

    if (pairs.length === 0) {
        similarityResults.innerHTML = `
            <div class="empty-state">
                <p>Add at least two documents to compare similarity.</p>
            </div>
        `;
        return;
    }

    similarityResults.innerHTML = pairs.map(pair => `
        <article class="similarity-card">
            <div class="pair-title">
                <span>${escapeHTML(pair.documentA)}</span>
                <span class="versus">VS</span>
                <span>${escapeHTML(pair.documentB)}</span>
            </div>
            <div class="similarity-value">
                ${pair.similarity.toFixed(2)}<small>%</small>
            </div>
            <div class="progress-track">
                <div class="progress-fill"
                     style="width: ${Math.min(100, pair.similarity)}%">
                </div>
            </div>
            <p>Jaccard similarity based on unique words</p>
        </article>
    `).join("");
}

document.getElementById("analyzeButton").addEventListener(
    "click",
    analyzeDocuments
);

// Upload elements.
const documentUpload = document.getElementById("documentUpload");
const uploadButton = document.getElementById("uploadButton");
const uploadStatus = document.getElementById("uploadStatus");

// Upload .txt files to the Node.js backend.
uploadButton.addEventListener("click", async () => {
    const files = Array.from(documentUpload.files);

    if (files.length === 0) {
        uploadStatus.textContent = "Please select at least one .txt file.";
        return;
    }

    const formData = new FormData();

    files.forEach(file => {
        formData.append("documents", file);
    });

    uploadButton.disabled = true;
    uploadStatus.textContent = "Uploading documents...";

    try {
        const response = await fetch("http://localhost:3000/api/upload", {
            method: "POST",
            body: formData
        });

        const data = await response.json();

        if (!response.ok) {
            throw new Error(data.error || "Upload failed.");
        }

        let added = 0;
        let skipped = 0;

        for (const uploadedFile of data.files) {
            const originalFile = files.find(
                file => file.name === uploadedFile.name
            );

            if (!originalFile) {
                continue;
            }

            const alreadyExists = documents.some(
                document =>
                    document.storedName &&
                    document.storedName === uploadedFile.storedName
            );

            if (alreadyExists) {
                skipped++;
                continue;
            }

            documents.push({
                name: uploadedFile.name,
                content: await originalFile.text(),
                storedName: uploadedFile.storedName
            });

            added++;
        }

        renderDocumentLibrary();

        uploadStatus.textContent =
            `${added} document${added === 1 ? "" : "s"} added successfully. ` +
            `Total documents: ${documents.length}.` +
            (skipped ? ` ${skipped} already added.` : "");

        documentUpload.value = "";

    } catch (error) {
        uploadStatus.textContent = `Upload failed: ${error.message}`;
    } finally {
        uploadButton.disabled = false;
    }
});

// Library elements.
const documentList = document.getElementById("documentList");
const libraryCount = document.getElementById("libraryCount");

// Render the Library.
function renderDocumentLibrary() {
    document.getElementById("documentCount").textContent =
        documents.length;

    libraryCount.textContent =
        `${documents.length} document${documents.length === 1 ? "" : "s"}`;

    documentList.innerHTML = "";

    if (documents.length === 0) {
        documentList.innerHTML = `
            <div class="empty-state">
                <p>No documents in your library. Add a .txt file to get started.</p>
            </div>
        `;
        return;
    }

    documents.forEach((doc, index) => {
        const card = document.createElement("article");
        card.className = "document-library-card";

        const details = document.createElement("div");
        details.className = "document-library-details";

        const name = document.createElement("h4");
        name.textContent = doc.name;

        const wordCount =
            doc.content.match(/[a-z0-9]+/gi)?.length || 0;

        const info = document.createElement("p");
        info.textContent = `${wordCount} words`;

        const removeButton = document.createElement("button");
        removeButton.type = "button";
        removeButton.className = "remove-document-button";
        removeButton.textContent = "Remove";

        removeButton.addEventListener("click", async () => {
            if (doc.storedName) {
                removeButton.disabled = true;

                try {
                    const response = await fetch(
                        `http://localhost:3000/api/documents/${encodeURIComponent(doc.storedName)}`,
                        { method: "DELETE" }
                    );

                    const data = response.status === 204
                        ? {}
                        : await response.json();

                    if (!response.ok) {
                        throw new Error(
                            data.error || "Document removal failed."
                        );
                    }
                } catch (error) {
                    uploadStatus.textContent =
                        `Could not remove document: ${error.message}`;

                    removeButton.disabled = false;
                    return;
                }
            }

            documents.splice(index, 1);
            renderDocumentLibrary();
        });

        details.append(name, info);
        card.append(details, removeButton);
        documentList.appendChild(card);
    });
}

// Initial render.
renderDocumentLibrary();

async function loadUploadedDocuments() {
    try {
        const response = await fetch(
            "http://localhost:3000/api/documents"
        );

        const data = await response.json();

        if (!response.ok) {
            throw new Error(
                data.error || "Document list request failed."
            );
        }

        for (const uploadedDocument of data.files) {
            const alreadyExists = documents.some(
                document =>
                    document.storedName === uploadedDocument.storedName
            );

            if (!alreadyExists) {
                documents.push(uploadedDocument);
            }
        }

        renderDocumentLibrary();

    } catch (error) {
        uploadStatus.textContent =
            `Could not load saved documents: ${error.message}`;
    }
}

loadUploadedDocuments();

// ------------------------------------------
// PERFORMANCE BENCHMARK
// ------------------------------------------

const benchmarkButton =
    document.getElementById("benchmarkButton");

const benchmarkStatus =
    document.getElementById("benchmarkStatus");

const benchmarkResults =
    document.getElementById("benchmarkResults");

if (benchmarkButton && benchmarkStatus && benchmarkResults) {
    benchmarkButton.addEventListener("click", async () => {
        // Use the current search keyword or "merge" as a default.
        const keyword = searchInput.value.trim() || "merge";

        benchmarkButton.disabled = true;
        benchmarkStatus.textContent =
            `Benchmarking the algorithms with "${keyword}"...`;

        benchmarkResults.innerHTML = `
            <div class="empty-state">
                <p>Running the C++ performance benchmark. Please wait...</p>
            </div>
        `;

        try {
            const response = await fetch(
                `http://localhost:3000/api/benchmark?keyword=${encodeURIComponent(keyword)}`
            );

            const data = await response.json();

            if (!response.ok) {
                throw new Error(
                    data.error || "Benchmark request failed."
                );
            }

            if (!Array.isArray(data.results) || data.results.length === 0) {
                throw new Error("No benchmark results were returned.");
            }

            const validResults = data.results.every(result =>
                typeof result.algorithm === "string" &&
                typeof result.time === "number" &&
                Number.isFinite(result.time) &&
                result.time >= 0
            );

            if (!validResults) {
                throw new Error("The benchmark returned invalid timings.");
            }

            const fastestTime = Math.min(
                ...data.results.map(result => result.time)
            );

            benchmarkStatus.textContent =
                `Benchmark completed for "${keyword}" — ` +
                `${data.repetitions || 10000} repetitions per algorithm.`;

            benchmarkResults.innerHTML = data.results.map(result => {
                const isFastest = result.time === fastestTime;

                // The bar is a relative comparison of measured times.
                const barWidth = fastestTime === 0
                    ? 100
                    : Math.max(
                        2,
                        (result.time / Math.max(
                            ...data.results.map(item => item.time),
                            1
                        )) * 100
                    );

                return `
                    <article class="result-card benchmark-result-card">
                        <div>
                            <h4>${escapeHTML(result.algorithm)}</h4>
                            <p>
                                ${result.time.toLocaleString()} 
                                ${escapeHTML(data.unit || "microseconds")}
                            </p>
                            <div class="progress-track">
                                <div class="progress-fill"
                                     style="width: ${barWidth}%">
                                </div>
                            </div>
                        </div>
                        ${
                            isFastest
                                ? '<span class="result-badge">Fastest</span>'
                                : ""
                        }
                    </article>
                `;
            }).join("");

        } catch (error) {
            benchmarkStatus.textContent = "Benchmark failed.";

            benchmarkResults.innerHTML = `
                <div class="empty-state">
                    <p>${escapeHTML(error.message)}</p>
                    <p>
                        Check that the backend is running and
                        main.exe supports --benchmark.
                    </p>
                </div>
            `;
        } finally {
            benchmarkButton.disabled = false;
        }
    });
}
