# Project Conventions

This document outlines the conventions we follow in our repository to maintain a clean, consistent, and efficient workflow. Adhering to these guidelines helps streamline development and collaboration.

## Commit Conventions

Following the Conventional Commits 1.0.0 specification, our commit messages aim to be both human and machine-readable, which facilitates automated tools and a clear project history.

### Key Points
- **Structure**: `<type>[optional scope]: <description> [optional body] [optional footer(s)]`
- **Types**: Use `fix:` for patches (correlates with PATCH in SemVer) and `feat:` for new features (correlates with MINOR in SemVer).
- **Breaking Changes**: Indicated by `BREAKING CHANGE:` in the footer or an exclamation mark `!` after the type/scope.
- **Scope**: Optionally includes additional context, enclosed in parentheses (e.g., `feat(parser):`).
- **Body & Footer**: The body is free-form text; footers follow a token-value format.

## Type
Must be one of the following:

- `build`: Changes that affect the build system or external dependencies (example scopes: gulp, broccoli, npm)
- `ci`: Changes to our CI configuration files and scripts (example scopes: Travis, Circle, BrowserStack, SauceLabs)
- `docs`: Documentation only changes
- `feat`: A new feature
- `fix`: A bug fix
- `perf`: A code change that improves performance
- `refactor`: A code change that neither fixes a bug nor adds a feature
- `style`: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc)
- `test`: Adding missing tests or correcting existing tests


### Benefits
- Automated CHANGELOG generation.
- Semantic versioning alignment.
- Clear communication of changes.
- Simplified contribution process.
joisdhnvkesadjvbnikegwqjkgvbqenijfgnwialjrbgeilrewbgjae;ilk
## Branch Naming Conventions

Branches in our repository follow a clear naming pattern that describes their purpose and context:

### Basic Rules
- Use **lowercase** and **hyphen-separated** names (e.g., `feature/new-login`).
- Stick to **alphanumeric characters** and hyphens, avoiding other punctuation.
- Avoid continuous or trailing hyphens.

### Branch Prefixes
- `feature/` for new features (e.g., `feature/login-system`).
- `bugfix/` for code fixes (e.g., `bugfix/header-styling`).
- `hotfix/` for urgent fixes (e.g., `hotfix/critical-security-issue`).
- `release/` for release preparation (e.g., `release/v1.0.1`).
- `docs/` for documentation updates (e.g., `docs/api-endpoints`).
- `test/` for testing ideas out (e.g., `test/new-control-algorithm`).

### Including Ticket Numbers
- Include ticket numbers for better tracking (e.g., `feature/T-123-new-login-system`).

## Coding Style

We follow the C# at Google Style Guide with some key points summarized below:

### Naming Conventions
- `PascalCase` for classes, methods, enums, public fields, properties, and namespaces.
- `camelCase` for local variables, parameters, and non-public fields and properties.
- Prefix interface names with `I` (e.g., `IInterface`).

### File Structure
- Filenames and directories in `PascalCase`.
- One core class per file, matching the filename.

### Organization
- Order of modifiers: `public protected internal private new abstract virtual override sealed static readonly extern unsafe volatile async`.
- `using` declarations at the top, `System` imports first, followed by others in alphabetical order.

### Whitespace and Wrapping
- Indent with 2 spaces (no tabs).
- Column limit of 100 characters.
- Braces used even when optional, with rules for line breaks and spacing.

### Comments and Documentation
- Use comments for non-obvious code features.
- Document public APIs with XML comments.

## Pull Request Process

To contribute to the project, follow these steps for submitting pull requests:

1. `Fork & Clone`: Fork the repository and clone it locally.
2. `Create a New Branch`: Switch to a new branch for your feature or fix.
3. `Make Your Changes`: Implement changes and commit with a clear message.
4. `Push Changes`: Push to your fork.
5. `Create the Pull Request`: On GitHub, select your branch, and provide a detailed description.
6. `Review & Discussion`: Participate in discussions on your PR.
7. `Update Your PR`: Make requested changes and push updates.
8. `Merge`: After approval, your PR will be merged.

Include tests, updated documentation, and screenshots (if applicable) in your PR.

Thank you for contributing to our project!
