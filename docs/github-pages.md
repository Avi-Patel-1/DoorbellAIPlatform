# GitHub Pages setup

Porchlight publishes the `docs-site/` Vite build with `.github/workflows/pages.yml`.

## First run in a new repository

Before rerunning the workflow, enable Pages in the repository UI:

1. Open the repository on GitHub.
2. Go to **Settings → Pages**.
3. Under **Build and deployment**, set **Source** to **GitHub Actions**.
4. Save.
5. Rerun the `pages / build` workflow from the Actions tab.

The workflow cannot reliably create the Pages site with the default `GITHUB_TOKEN`. Once the repository is configured to use GitHub Actions as the Pages source, the workflow builds `docs-site/`, uploads the static artifact, and deploys it to the `github-pages` environment.

## Local verification

```bash
make assets
make docs-build
cd docs-site
npm run preview
```

For repository Pages URLs such as `https://<owner>.github.io/<repo>/`, the workflow sets `PORCHLIGHT_DOCS_BASE` to `/<repo>/` so Vite asset paths resolve correctly.
