
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

// ------------------------------------------
// TEXT PROCESSING AND SIMILARITY
// ------------------------------------------

// Calculate LCS token similarity as a percentage.
function calculateLcsSimilarity(textA, textB) {
    const wordsA = String(textA || "").toLowerCase().match(/[a-z0-9]+/g) || [];
    const wordsB = String(textB || "").toLowerCase().match(/[a-z0-9]+/g) || [];
    const maxTokens = 3000;
    wordsA.length = Math.min(wordsA.length, maxTokens);
    wordsB.length = Math.min(wordsB.length, maxTokens);

    if (wordsA.length === 0 && wordsB.length === 0) {
        return 100;
    }

    let previous = new Array(wordsB.length + 1).fill(0);
    let current = new Array(wordsB.length + 1).fill(0);

    for (let i = 1; i <= wordsA.length; i++) {
        for (let j = 1; j <= wordsB.length; j++) {
            current[j] = wordsA[i - 1] === wordsB[j - 1]
                ? previous[j - 1] + 1
                : Math.max(previous[j], current[j - 1]);
        }
        [previous, current] = [current, previous];
        current.fill(0);
    }

    const comparedLength = wordsA.length + wordsB.length;
    return comparedLength === 0
        ? 0
        : (200 * previous[wordsB.length]) / comparedLength;
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

// Highlight keyword matches safely in document text.
function highlightMatches(text, keyword) {
    if (!keyword || !keyword.trim()) {
        return escapeHTML(text);
    }

    const escapedKeyword = keyword.trim().replace(
        /[.*+?^${}()|[\]\\]/g,
        "\\$&"
    );

    const regex = new RegExp(`(${escapedKeyword})`, "gi");

    return String(text)
        .split(regex)
        .map(part => {
            if (part.toLowerCase() === keyword.trim().toLowerCase()) {
                return `<mark>${escapeHTML(part)}</mark>`;
            }

            return escapeHTML(part);
        })
        .join("");
}

function createSearchPreview(text, keyword, contextLength = 100) {
    const content = String(text || "");
    const query = String(keyword || "").trim();

    if (!content) {
        return "No document content available.";
    }

    if (!query) {
        return escapeHTML(content.slice(0, contextLength * 2));
    }

    const index = content.toLowerCase().indexOf(query.toLowerCase());

    // If the exact phrase isn't found, show the beginning of the document.
    if (index === -1) {
        const preview = content.slice(0, contextLength * 2);
        return escapeHTML(preview) +
            (content.length > preview.length ? "..." : "");
    }

    const start = Math.max(0, index - contextLength);
    const end = Math.min(
        content.length,
        index + query.length + contextLength
    );

    const preview = content.slice(start, end);

    return (
        (start > 0 ? "..." : "") +
        highlightMatches(preview, query) +
        (end < content.length ? "..." : "")
    );
}



// Return a stable key for identifying a document.
function getDocumentKey(doc) {
    return doc.storedName || doc.name;
}

// ------------------------------------------
// SEARCH DOCUMENTS
// ------------------------------------------

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
            // Normalize Windows paths and extract the stored filename.
            const normalizedPath = filename.replace(/\\/g, "/");
            const basename = normalizedPath.split("/").pop();

            // Match the C++ result against the frontend document library.
            const matchedDocument = documents.find(item => {
                const possibleNames = [
                    item.name,
                    item.storedName
                ].filter(Boolean);

                return possibleNames.some(name =>
                    name === basename ||
                    basename.endsWith("-" + name)
                );
            });

            const displayName = matchedDocument
                ? matchedDocument.name
                : basename;

            const content = matchedDocument
                ? matchedDocument.content
                : "";

            const preview = createSearchPreview(
                content,
                normalizedKeyword
            );

            return `
        <article class="result-card">
            <div>
                <h4>${escapeHTML(displayName)}</h4>
                <p>${preview}</p>
                ${!matchedDocument ? `
                    <small>
                        Search matched this file in C++, but its
                        content is not available in the frontend.
                    </small>
                ` : ""}
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

// ------------------------------------------
// DOCUMENT SIMILARITY
// ------------------------------------------

const similarityResults =
    document.getElementById("similarityResults");

const analyzeButton =
    document.getElementById("analyzeButton");

const similarityDocumentA =
    document.getElementById("similarityDocumentA");

const similarityDocumentB =
    document.getElementById("similarityDocumentB");

const compareDocumentsButton =
    document.getElementById("compareDocumentsButton");

const pairSimilarityStatus =
    document.getElementById("pairSimilarityStatus");

const pairSimilarityResult =
    document.getElementById("pairSimilarityResult");

const similarityPairCount =
    document.getElementById("similarityPairCount");

const similarityStatus =
    document.getElementById("similarityStatus");

// Fill both dropdowns with documents currently in the library.
function renderSimilaritySelectors() {
    if (!similarityDocumentA || !similarityDocumentB) {
        return;
    }

    const previousA = similarityDocumentA.value;
    const previousB = similarityDocumentB.value;

    const options = documents.map(doc => {
        const key = getDocumentKey(doc);

        return `
            <option value="${escapeHTML(key)}">
                ${escapeHTML(doc.name)}
            </option>
        `;
    }).join("");

    similarityDocumentA.innerHTML =
        '<option value="">Select first document</option>' + options;

    similarityDocumentB.innerHTML =
        '<option value="">Select second document</option>' + options;

    if (documents.some(doc => getDocumentKey(doc) === previousA)) {
        similarityDocumentA.value = previousA;
    }

    if (documents.some(doc => getDocumentKey(doc) === previousB)) {
        similarityDocumentB.value = previousB;
    }

    updateSecondDocumentOptions();
}

// Prevent selecting the same document in both dropdowns.
function updateSecondDocumentOptions() {
    if (!similarityDocumentA || !similarityDocumentB) {
        return;
    }

    const selectedA = similarityDocumentA.value;
    const selectedB = similarityDocumentB.value;

    Array.from(similarityDocumentB.options).forEach(option => {
        option.disabled =
            option.value !== "" &&
            option.value === selectedA;
    });

    if (selectedA && selectedA === selectedB) {
        similarityDocumentB.value = "";
    }

    Array.from(similarityDocumentA.options).forEach(option => {
        option.disabled =
            option.value !== "" &&
            option.value === similarityDocumentB.value;
    });
}

// Find a document by its stable key.
function findDocumentByKey(key) {
    return documents.find(doc => getDocumentKey(doc) === key);
}

// Build a similarity card.
function createSimilarityCard(documentA, documentB, similarity) {
    const percentage = Math.min(100, Math.max(0, similarity));

    return `
        <article class="similarity-card">
            <div class="pair-title">
                <span>${escapeHTML(documentA.name)}</span>
                <span class="versus">VS</span>
                <span>${escapeHTML(documentB.name)}</span>
            </div>

            <div class="similarity-value">
                ${similarity.toFixed(2)}<small>%</small>
            </div>

            <div class="progress-track">
                <div class="progress-fill"
                     style="width: ${percentage}%">
                </div>
            </div>

            <p>LCS similarity based on word order</p>
        </article>
    `;
}

// Compare the two selected documents.

async function compareSelectedDocuments() {
    if (
        !similarityDocumentA ||
        !similarityDocumentB ||
        !pairSimilarityStatus ||
        !pairSimilarityResult
    ) {
        return;
    }

    const keyA = similarityDocumentA.value;
    const keyB = similarityDocumentB.value;

    if (!keyA || !keyB) {
        pairSimilarityStatus.textContent =
            "Please select both documents before comparing.";
        pairSimilarityResult.textContent = "";
        return;
    }

    if (keyA === keyB) {
        pairSimilarityStatus.textContent =
            "Please select two different documents.";
        pairSimilarityResult.textContent = "";
        return;
    }

    const documentA = findDocumentByKey(keyA);
    const documentB = findDocumentByKey(keyB);

    if (!documentA || !documentB) {
        pairSimilarityStatus.textContent =
            "One of the selected documents is no longer available.";
        pairSimilarityResult.textContent = "";
        renderSimilaritySelectors();
        return;
    }

    pairSimilarityStatus.textContent =
        "Calculating similarity...";
    pairSimilarityResult.textContent = "";

    try {
        const response = await fetch("http://localhost:3000/api/similarity", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({
                document1: documentA.storedName || documentA.name,
                document2: documentB.storedName || documentB.name
            })
        });

        const responseText = await response.text();

        let data;

        try {
            data = JSON.parse(responseText);
        } catch (error) {
            console.error("Invalid backend response:", responseText);
            throw new Error(
                "Backend returned an empty or invalid response. Check the backend terminal."
            );
        }

        if (!response.ok) {
            throw new Error(
                data.error || "Similarity calculation failed."
            );
        }

        pairSimilarityStatus.textContent =
            `Comparison complete: ${documentA.name} vs ${documentB.name}.`;

        pairSimilarityResult.textContent =
            `LCS Similarity: ${Number(data.lcsSimilarity).toFixed(2)}%`;

        pairSimilarityResult.style.whiteSpace = "pre-line";

    } catch (error) {
        pairSimilarityStatus.textContent =
            "Could not calculate document similarity.";
        pairSimilarityResult.textContent = error.message;
    }
}


// Compare every unique pair of documents.
function analyzeDocuments() {
    if (!similarityResults) {
        return;
    }

    const pairs = [];

    for (let i = 0; i < documents.length; i++) {
        for (let j = i + 1; j < documents.length; j++) {
            const similarity = calculateLcsSimilarity(
                documents[i].content,
                documents[j].content
            );

            pairs.push({
                documentA: documents[i],
                documentB: documents[j],
                similarity
            });
        }
    }

    // Show the most similar pairs first.
    pairs.sort((a, b) => b.similarity - a.similarity);

    if (similarityPairCount) {
        similarityPairCount.textContent =
            `${pairs.length} pair${pairs.length === 1 ? "" : "s"}`;
    }

    if (similarityStatus) {
        similarityStatus.textContent =
            `Analysis complete. Compared ${documents.length} documents across ${pairs.length} unique pairs.`;
    }

    if (pairs.length === 0) {
        similarityResults.innerHTML = `
            <div class="empty-state">
                <p>Add at least two documents to compare similarity.</p>
            </div>
        `;
        return;
    }

    similarityResults.innerHTML = pairs.map(pair =>
        createSimilarityCard(
            pair.documentA,
            pair.documentB,
            pair.similarity
        )
    ).join("");
}

if (analyzeButton) {
    analyzeButton.addEventListener("click", analyzeDocuments);
}

if (compareDocumentsButton) {
    compareDocumentsButton.addEventListener(
        "click",
        compareSelectedDocuments
    );
}

if (similarityDocumentA) {
    similarityDocumentA.addEventListener(
        "change",
        updateSecondDocumentOptions
    );
}

if (similarityDocumentB) {
    similarityDocumentB.addEventListener(
        "change",
        updateSecondDocumentOptions
    );
}

// ------------------------------------------
// UPLOAD DOCUMENTS
// ------------------------------------------

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
                doc =>
                    doc.storedName &&
                    doc.storedName === uploadedFile.storedName
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

// ------------------------------------------
// DOCUMENT LIBRARY
// ------------------------------------------

const documentList = document.getElementById("documentList");
const libraryCount = document.getElementById("libraryCount");

// Render the library and refresh similarity selectors.
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
    } else {
        documents.forEach(doc => {
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

                const index = documents.indexOf(doc);

                if (index !== -1) {
                    documents.splice(index, 1);
                }

                renderDocumentLibrary();

                if (similarityStatus) {
                    similarityStatus.textContent =
                        "Library updated. Run analysis again to refresh all pairs.";
                }

                if (pairSimilarityResult) {
                    pairSimilarityResult.innerHTML = "";
                }

                if (pairSimilarityStatus) {
                    pairSimilarityStatus.textContent =
                        "Select two documents to compare.";
                }
            });

            details.append(name, info);
            card.append(details, removeButton);
            documentList.appendChild(card);
        });
    }

    renderSimilaritySelectors();
}

// Initial render.
renderDocumentLibrary();

// ------------------------------------------
// LOAD SAVED UPLOADS
// ------------------------------------------

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
                doc =>
                    doc.storedName === uploadedDocument.storedName
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
const benchmarkCharts =
    document.getElementById("benchmarkCharts");

if (benchmarkButton && benchmarkStatus && benchmarkResults) {
    benchmarkButton.addEventListener("click", async () => {
        const requestedKeyword = searchInput.value.trim() || "merge";

        benchmarkButton.disabled = true;
        benchmarkStatus.textContent =
            "Benchmarking all three input sizes...";

        benchmarkResults.innerHTML = `
            <div class="empty-state">
                <p>Running C++ performance tests. Please wait...</p>
            </div>
        `;

        try {
            const response = await fetch(
                `http://localhost:3000/api/benchmark?keyword=${encodeURIComponent(requestedKeyword)}`
            );

            const data = await response.json();

            if (!response.ok) {
                throw new Error(
                    data.error || "Benchmark request failed."
                );
            }

            const results = data.results;

            if (!Array.isArray(results) || results.length !== 3) {
                throw new Error(
                    "Expected benchmark results for small, medium, and large inputs."
                );
            }

            const expectedSizes = ["small", "medium", "large"];

            for (const size of expectedSizes) {
                if (!results.some(result => result.size === size)) {
                    throw new Error(`Missing ${size} input results.`);
                }
            }

            const requiredFields = [
                "characters",
                "repetitions",
                "linearSearch",
                "kmpSearch",
                "boyerMooreSearch",
                "linearAverage",
                "kmpAverage",
                "boyerMooreAverage"
            ];

            for (const result of results) {
                for (const field of requiredFields) {
                    if (
                        typeof result[field] !== "number" ||
                        !Number.isFinite(result[field]) ||
                        result[field] < 0
                    ) {
                        throw new Error(
                            `Invalid ${field} value for ${result.size} input.`
                        );
                    }
                }
            }

            benchmarkStatus.textContent =
                `Benchmark completed. Search keyword: "${data.keyword}". ` +
                `Times are measured in ${data.unit || "microseconds"}.`;

            const algorithms = [
                {
                    key: "linearSearch",
                    averageKey: "linearAverage",
                    name: "Naive (Linear) Search"
                },
                {
                    key: "kmpSearch",
                    averageKey: "kmpAverage",
                    name: "KMP Search"
                },
                {
                    key: "boyerMooreSearch",
                    averageKey: "boyerMooreAverage",
                    name: "Boyer-Moore"
                }
            ];

            const formatTime = value =>
                Number(value).toLocaleString(undefined, {
                    maximumFractionDigits: 4
                });

            if (benchmarkCharts) {
                const chartAlgorithms = [
                    {
                        key: "linearAverage",
                        name: "Linear Search",
                        className: "benchmark-bar-linear"
                    },
                    {
                        key: "kmpAverage",
                        name: "KMP Search",
                        className: "benchmark-bar-kmp"
                    },
                    {
                        key: "boyerMooreAverage",
                        name: "Boyer-Moore",
                        className: "benchmark-bar-boyer-moore"
                    }
                ];

                benchmarkCharts.innerHTML = results.map(result => {
                    const maxTime = Math.max(
                        ...chartAlgorithms.map(algorithm =>
                            Number(result[algorithm.key]) || 0
                        )
                    );

                    const sizeLabel =
                        String(result.size).charAt(0).toUpperCase() +
                        String(result.size).slice(1);

                    const bars = chartAlgorithms.map(algorithm => {
                        const average = Number(result[algorithm.key]) || 0;

                        const width = maxTime > 0
                            ? (average / maxTime) * 100
                            : 0;

                        return `
                <div class="benchmark-bar-group">
                    <div class="benchmark-bar-label">
                        ${escapeHTML(algorithm.name)}
                    </div>

                    <div class="benchmark-bar-stack">
                        <div class="benchmark-bar-row">
                            <div class="benchmark-bar-track">
                                <div
                                    class="benchmark-bar ${algorithm.className}"
                                    style="width: ${width}%"
                                ></div>
                            </div>

                            <span class="benchmark-bar-value">
                                ${formatTime(average)}
                                ${escapeHTML(data.unit || "microseconds")}
                            </span>
                        </div>
                    </div>
                </div>
            `;
                    }).join("");

                    return `
            <section class="benchmark-chart-card">
                <h4>${escapeHTML(sizeLabel)} Input</h4>

                <p class="benchmark-chart-description">
                    ${Number(result.characters).toLocaleString()}
                    characters · Average time per search
                </p>

                <div class="benchmark-chart-legend">
                    ${chartAlgorithms.map(algorithm => `
                        <span class="benchmark-legend-item">
                            <span class="benchmark-legend-dot
                                ${algorithm.className}"></span>
                            ${escapeHTML(algorithm.name)}
                        </span>
                    `).join("")}
                </div>

                ${bars}

                <p class="benchmark-chart-note">
                    Bars are scaled relative to the slowest algorithm
                    for this input size. Shorter bars indicate faster
                    measured execution.
                </p>
            </section>
        `;
                }).join("");
            }


            benchmarkResults.innerHTML = results.map(result => {
                const fastestAlgorithm = algorithms.reduce(
                    (fastest, algorithm) =>
                        result[algorithm.averageKey] <
                            result[fastest.averageKey]
                            ? algorithm
                            : fastest,
                    algorithms[0]
                );

                const slowestAverage = Math.max(
                    ...algorithms.map(
                        algorithm => result[algorithm.averageKey]
                    ),
                    0
                );

                const cards = algorithms.map(algorithm => {
                    const average = result[algorithm.averageKey];
                    const total = result[algorithm.key];
                    const isFastest = algorithm === fastestAlgorithm;

                    const barWidth = slowestAverage === 0
                        ? 0
                        : (average / slowestAverage) * 100;

                    return `
                        <article class="result-card benchmark-result-card">
                            <div>
                                <h4>${escapeHTML(algorithm.name)}</h4>
                                <p>
                                    <strong>${formatTime(average)}</strong>
                                    ${escapeHTML(data.unit || "microseconds")} per search
                                </p>
                                <p>
                                    Total: ${formatTime(total)}
                                    ${escapeHTML(data.unit || "microseconds")}
                                    across ${result.repetitions.toLocaleString()} runs
                                </p>
                                <div class="progress-track">
                                    <div class="progress-fill"
                                         style="width: ${barWidth}%">
                                    </div>
                                </div>
                            </div>
                            ${isFastest
                            ? '<span class="result-badge">Fastest</span>'
                            : ""
                        }
                        </article>
                    `;
                }).join("");

                return `
                    <section class="benchmark-size-section">
                        <h3>${escapeHTML(
                    result.size.charAt(0).toUpperCase() +
                    result.size.slice(1)
                )} Input</h3>

                        <p class="benchmark-size-details">
                            ${result.characters.toLocaleString()} characters
                            · ${result.repetitions.toLocaleString()} repetitions per algorithm
                        </p>

                        <div class="benchmark-algorithm-results">
                            ${cards}
                        </div>

                        <p class="benchmark-winner">
                            Fastest for this input:
                            <strong>${escapeHTML(fastestAlgorithm.name)}</strong>
                        </p>
                    </section>
                `;
            }).join("");

        } catch (error) {
            benchmarkStatus.textContent = "Benchmark failed.";

            benchmarkResults.innerHTML = `
                <div class="empty-state">
                    <p>${escapeHTML(error.message)}</p>
                    <p>
                        Check that the backend is running and the C++
                        benchmark returns all three input sizes.
                    </p>
                </div>
            `;
        } finally {
            benchmarkButton.disabled = false;
        }
    });
}
