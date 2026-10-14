document.addEventListener('DOMContentLoaded', () => {
    const searchInput = document.getElementById('searchInput');
    const autocompleteList = document.getElementById('autocompleteList');
    const searchBtn = document.getElementById('searchBtn');
    const filterLanguage = document.getElementById('filterLanguage');
    const filterStars = document.getElementById('filterStars');
    const filterDocker = document.getElementById('filterDocker');
    const resultsContainer = document.getElementById('resultsContainer');
    const languageAutocompleteList = document.getElementById('languageAutocompleteList');

    const commonLanguages = ["Go", "Python", "JavaScript", "TypeScript", "Java", "C++", "C", "C#", "Ruby", "PHP", "Rust", "Swift", "Kotlin"];

    let debounceTimer;

    // Strip emojis and leading/trailing whitespace from a string
    function stripEmojis(str) {
        return str
            .replace(/[\u{1F000}-\u{1FFFF}]/gu, '')
            .replace(/[\u{2600}-\u{27BF}]/gu, '')
            .replace(/[\u{FE00}-\u{FEFF}]/gu, '')
            .replace(/[\u{1F900}-\u{1F9FF}]/gu, '')
            .replace(/[\uD800-\uDBFF][\uDC00-\uDFFF]/g, '')
            .replace(/[\u2000-\u206F]/g, '')
            .replace(/[\u2300-\u23FF]/g, '')
            .replace(/\s+/g, ' ')
            .trim();
    }

    // ── Search bar autocomplete ──────────────────────────────────────────────
    searchInput.addEventListener('input', (e) => {
        clearTimeout(debounceTimer);
        const prefix = e.target.value.trim();

        if (!prefix) {
            autocompleteList.classList.add('hidden');
            return;
        }

        debounceTimer = setTimeout(async () => {
            try {
                const res = await fetch(`http://localhost:45678/api/autocomplete?prefix=${encodeURIComponent(prefix)}`);
                if (!res.ok) return;
                const data = await res.json();

                autocompleteList.innerHTML = '';
                if (data.suggestions && data.suggestions.length > 0) {
                    data.suggestions.forEach(s => {
                        const li = document.createElement('li');
                        li.textContent = s;
                        li.addEventListener('click', () => {
                            searchInput.value = s;
                            autocompleteList.classList.add('hidden');
                            performSearch();
                        });
                        autocompleteList.appendChild(li);
                    });
                    autocompleteList.classList.remove('hidden');
                } else {
                    autocompleteList.classList.add('hidden');
                }
            } catch (err) {
                console.error("Autocomplete error:", err);
            }
        }, 200);
    });

    // ── Language filter autocomplete ─────────────────────────────────────────
    filterLanguage.addEventListener('input', (e) => {
        const prefix = e.target.value.trim().toLowerCase();
        languageAutocompleteList.innerHTML = '';
        if (!prefix) {
            languageAutocompleteList.classList.add('hidden');
            return;
        }

        const matches = commonLanguages.filter(l => l.toLowerCase().startsWith(prefix));
        if (matches.length > 0) {
            matches.forEach(s => {
                const li = document.createElement('li');
                li.textContent = s;
                li.addEventListener('click', () => {
                    filterLanguage.value = s.toLowerCase();
                    languageAutocompleteList.classList.add('hidden');
                    performSearch();
                });
                languageAutocompleteList.appendChild(li);
            });
            languageAutocompleteList.classList.remove('hidden');
        } else {
            languageAutocompleteList.classList.add('hidden');
        }
    });

    // ── Close dropdowns on outside click ─────────────────────────────────────
    document.addEventListener('click', (e) => {
        if (!e.target.closest('.search-input-wrapper')) {
            autocompleteList.classList.add('hidden');
        }
        if (!e.target.closest('.filter-input-wrapper')) {
            languageAutocompleteList.classList.add('hidden');
        }
    });

    // ── Search triggers ───────────────────────────────────────────────────────
    searchBtn.addEventListener('click', performSearch);
    searchInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') performSearch();
    });

    // ── Core search function ──────────────────────────────────────────────────
    async function performSearch() {
        autocompleteList.classList.add('hidden');
        resultsContainer.innerHTML = '<div class="text-muted">Searching...</div>';

        let q = searchInput.value.trim();
        if (!q) {
            resultsContainer.innerHTML = '<div class="text-muted">Please enter a search query.</div>';
            return;
        }

        if (filterLanguage.value.trim()) q += ` language:${filterLanguage.value.trim()}`;
        if (filterDocker.checked) q += ` has:docker`;
        if (filterStars.value.trim()) q += ` stars:>${filterStars.value.trim()}`;

        try {
            const res = await fetch(`http://localhost:45678/api/search?q=${encodeURIComponent(q)}`);
            if (!res.ok) {
                const errorData = await res.json();
                resultsContainer.innerHTML = `<div class="text-muted">Error: ${errorData.error || 'Unknown error'}</div>`;
                return;
            }

            const data = await res.json();
            renderResults(data.results || []);
        } catch (err) {
            resultsContainer.innerHTML = `<div class="text-muted">Network error: Could not reach backend server on port 45678.</div>`;
            console.error("Search error:", err);
        }
    }

    // ── Render results ────────────────────────────────────────────────────────
    function renderResults(results) {
        resultsContainer.innerHTML = '';
        if (results.length === 0) {
            resultsContainer.innerHTML = '<div class="text-muted">No results found.</div>';
            return;
        }

        results.forEach(item => {
            const el = document.createElement('div');
            el.className = 'result-item';

            const bd = item.breakdown;
            const fmt = (val) => Number(val || 0).toFixed(2);

            // Build GitHub link only if owner + name are present and non-empty
            let repoNameHtml;
            if (item.owner && item.name) {
                const url = `https://github.com/${item.owner}/${item.name}`;
                repoNameHtml = `<a href="${url}" target="_blank" rel="noopener noreferrer" class="repo-name">${item.repo_id}</a>`;
            } else {
                repoNameHtml = `<span class="repo-name">${item.repo_id}</span>`;
            }

            const descId = `desc-${item.repo_id.replace(/\//g, '-')}`;
            
            let matchedTermsHtml = '';
            if (item.missing_terms && item.missing_terms.length > 0) {
                matchedTermsHtml = `<div class="text-muted" style="margin-top: 8px;">Matched: ${item.matched_terms.join(', ')} &mdash; Not matched: ${item.missing_terms.join(', ')}</div>`;
            }

            el.innerHTML = `
                <div class="result-header">
                    ${repoNameHtml}
                    <div class="repo-score-block">
                        <span class="repo-score">Score: ${fmt(item.final_score)}</span>
                        <span class="repo-driven-by">driven by ${item.driven_by || 'relevance'}</span>
                    </div>
                </div>
                <div class="result-desc text-muted" id="${descId}">Loading description...</div>
                ${matchedTermsHtml}
                <button class="details-btn">Details</button>
                <div class="breakdown">
                    ${renderBar('Relevance',   'how well this result matches the search terms',        fmt(bd.relevance),   'bar-relevance')}
                    ${renderBar('Popularity',  'community adoption, based on stars and forks',         fmt(bd.popularity),  'bar-popularity')}
                    ${renderBar('Maintenance', 'how recently and actively the project is updated',     fmt(bd.maintenance), 'bar-maintenance')}
                    ${renderBar('DevOps',      'production-readiness: Docker, CI/CD, and tests present', fmt(bd.devops),    'bar-devops')}
                </div>
            `;

            const detailsBtn = el.querySelector('.details-btn');
            const breakdownEl = el.querySelector('.breakdown');

            detailsBtn.addEventListener('click', () => {
                breakdownEl.classList.toggle('visible');
                detailsBtn.textContent = breakdownEl.classList.contains('visible') ? 'Hide Details' : 'Details';
            });

            resultsContainer.appendChild(el);

            // Fetch description + language from GitHub API
            fetch(`https://api.github.com/repos/${item.repo_id}`)
                .then(r => r.json())
                .then(ghData => {
                    const descEl = document.getElementById(descId);
                    if (descEl) {
                        const rawDesc = ghData.description || 'No description provided.';
                        const description = stripEmojis(rawDesc);
                        const language = ghData.language || 'Unknown';
                        descEl.innerHTML = `${description} &middot; <strong>Language:</strong> ${language}`;
                    }
                })
                .catch(() => {
                    const descEl = document.getElementById(descId);
                    if (descEl) descEl.textContent = 'No description available.';
                });
        });
    }

    // ── Score bar helper ──────────────────────────────────────────────────────
    function renderBar(label, explanation, value, colorClass) {
        const width = Math.min(Math.max(Number(value) * 100, 2), 100);
        return `
            <div class="breakdown-row">
                <div class="breakdown-label">
                    <span class="breakdown-label-name">${label}</span>
                    <span class="breakdown-label-desc">${explanation}</span>
                </div>
                <div class="breakdown-bar-container">
                    <div class="breakdown-bar ${colorClass}" style="width:${width}%"></div>
                </div>
                <div class="breakdown-val">${value}</div>
            </div>
        `;
    }
});
