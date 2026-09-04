import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';

export const repositoryConfigPath = 'workspace-repositories.json';

function runGit(repositoryPath, args, { optional = false } = {}) {
  const result = spawnSync('git', ['-C', repositoryPath, ...args], {
    encoding: 'utf8',
    maxBuffer: 128 * 1024 * 1024,
  });
  if (result.status !== 0 && !optional) {
    throw new Error(result.stderr.trim() || `git ${args.join(' ')} failed in ${repositoryPath}.`);
  }
  return result.status === 0 ? result.stdout.trim() : null;
}

export function resolveRepository(entry, workspaceRoot, environment = process.env) {
  const override = entry.environmentVariable ? environment[entry.environmentVariable] : null;
  const configuredPath = override || entry.path;
  if (!configuredPath) throw new Error(`Repository '${entry.id}' has no path.`);
  const resolvedPath = path.resolve(workspaceRoot, configuredPath);
  const cacheRoot = path.resolve(workspaceRoot, 'analysis-cache');
  const authoritative = entry.role === 'public-product-source' || entry.role === 'private-release-governance';
  if (authoritative && (resolvedPath === cacheRoot || resolvedPath.startsWith(`${cacheRoot}${path.sep}`))) {
    throw new Error(`Authoritative repository '${entry.id}' cannot live under analysis-cache.`);
  }
  return { ...entry, path: resolvedPath, pathSource: override ? 'environment' : 'config' };
}

export function loadWorkspaceRepositories(workspaceRoot, environment = process.env) {
  const configFile = path.join(workspaceRoot, repositoryConfigPath);
  const config = JSON.parse(fs.readFileSync(configFile, 'utf8'));
  if (config.schemaVersion !== 1 || !Array.isArray(config.repositories)) {
    throw new Error(`${repositoryConfigPath} requires schemaVersion 1 and a repositories array.`);
  }
  const ids = config.repositories.map((entry) => entry.id);
  if (new Set(ids).size !== ids.length) throw new Error('Repository ids must be unique.');
  return config.repositories.map((entry) => resolveRepository(entry, workspaceRoot, environment));
}

export function inspectGitRepository(repository) {
  const exists = fs.existsSync(repository.path);
  const gitDirectoryExists = exists && fs.existsSync(path.join(repository.path, '.git'));
  if (!gitDirectoryExists) {
    return {
      ...repository,
      exists,
      gitRepository: false,
      branch: null,
      upstream: null,
      head: null,
      origin: null,
      changed: 0,
      staged: 0,
      unstaged: 0,
      untracked: 0,
    };
  }

  const status = (runGit(repository.path, ['status', '--porcelain=v1']) || '')
    .split(/\r?\n/)
    .filter(Boolean);
  let staged = 0;
  let unstaged = 0;
  let untracked = 0;
  for (const line of status) {
    if (line.startsWith('??')) {
      untracked += 1;
      continue;
    }
    if (line[0] && line[0] !== ' ') staged += 1;
    if (line[1] && line[1] !== ' ') unstaged += 1;
  }

  return {
    ...repository,
    exists: true,
    gitRepository: true,
    branch: runGit(repository.path, ['branch', '--show-current']) || null,
    upstream: runGit(repository.path, ['rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}'], { optional: true }),
    head: runGit(repository.path, ['rev-parse', 'HEAD']),
    origin: runGit(repository.path, ['remote', 'get-url', 'origin'], { optional: true }),
    changed: status.length,
    staged,
    unstaged,
    untracked,
  };
}

export function inspectWorkspaceRepositories(workspaceRoot, environment = process.env) {
  return loadWorkspaceRepositories(workspaceRoot, environment).map(inspectGitRepository);
}
